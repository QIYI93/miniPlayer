#include "MediaDisplay_Directx.h"
#include "util.h"


extern "C"
{
//#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#define RENDER_NEXT_FRAME (WM_USER + 0x100)
#define CLOSE_WINDOW (WM_USER + 0x101)
#define LOAD_FRAME (WM_USER + 0x102)
#define AUDIO_QUEUE_SIZE 4

namespace
{
    auto const audioMinBufferSize = 512;
    auto const audioMaxCallBackPerSec = 30;
}

static LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam);

MediaDisplay_Directx::MediaDisplay_Directx(MediaMainControl *mainCtrl)
    :MediaDisplay(mainCtrl)
    , m_direct3D9(nullptr, [](IDirect3D9 *p) {p->Release(); })
    , m_direct3DDevice(nullptr, [](IDirect3DDevice9 *p) {p->Release(); })
    , m_direct3DTexture(nullptr, [](IDirect3DTexture9 *p) {p->Release(); })
    , m_direct3DVertexBuffer(nullptr, [](IDirect3DVertexBuffer9 *p) {p->Release(); })
{
    m_displayType = DisplayType::USING_DIRECTX;
    InitializeCriticalSection(&m_critial);
}

MediaDisplay_Directx::~MediaDisplay_Directx()
{
}

bool MediaDisplay_Directx::init()
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = (WNDPROC)winProc;
    wc.lpszClassName = L"D3DWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassEx(&wc);

    m_mainWnd = CreateWindow(L"D3DWindow", L"", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (m_mainWnd == NULL)
        return false;

    return true;
}

bool MediaDisplay_Directx::initD3D(int width, int height)
{
    m_frameWidth = width;
    m_frameHeight = height;

    HRESULT lRet;
    m_direct3D9.reset(Direct3DCreate9(D3D_SDK_VERSION));
    if (m_direct3D9 == nullptr)
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create Direct3D9.\n");
        return false;
    }
    lRet = m_direct3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_d3dDisplayMode);
    if (FAILED(lRet))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to get display mode.\n");
        return false;
    }
    lRet = m_direct3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dcaps);
    if (FAILED(lRet))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to get device-specific information.\n");
        return false;
    }
    if (m_d3dcaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
        m_supportHardwareVP = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth = width;
    d3dpp.BackBufferHeight = height;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;  /*D3DFMT_X8R8G8B8*/
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
    d3dpp.hDeviceWindow = m_mainWnd;
    d3dpp.Windowed = TRUE;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    IDirect3DDevice9 *direct3DDevice;
    lRet = m_direct3D9->CreateDevice(
        D3DADAPTER_DEFAULT, 
        D3DDEVTYPE_HAL, 
        NULL,
        m_supportHardwareVP | D3DCREATE_MULTITHREADED,
        &d3dpp, 
        &direct3DDevice);

    if (FAILED(lRet))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create device.\n");
        return false;
    }
    m_direct3DDevice.reset(direct3DDevice);

    //SetSamplerState()  
    // Texture coordinates outside the range [0.0, 1.0] are set  
    // to the texture color at 0.0 or 1.0, respectively.  
    IDirect3DDevice9_SetSamplerState(m_direct3DDevice, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    IDirect3DDevice9_SetSamplerState(m_direct3DDevice, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    // Set linear filtering quality  
    IDirect3DDevice9_SetSamplerState(m_direct3DDevice, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    IDirect3DDevice9_SetSamplerState(m_direct3DDevice, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    //SetRenderState()  
    //set maximum ambient light  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_AMBIENT, D3DCOLOR_XRGB(255, 255, 0));
    // Turn off culling  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_CULLMODE, D3DCULL_NONE);
    // Turn off the zbuffer  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_ZENABLE, D3DZB_FALSE);
    // Turn off lights  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_LIGHTING, FALSE);
    // Enable dithering  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_DITHERENABLE, TRUE);
    // disable stencil  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_STENCILENABLE, FALSE);
    // manage blending  
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_ALPHABLENDENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_ALPHATESTENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_ALPHAREF, 0x10);
    IDirect3DDevice9_SetRenderState(m_direct3DDevice, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    // Set texture states  
    IDirect3DDevice9_SetTextureStageState(m_direct3DDevice, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(m_direct3DDevice, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(m_direct3DDevice, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    // turn off alpha operation  
    IDirect3DDevice9_SetTextureStageState(m_direct3DDevice, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    IDirect3DTexture9 *direct3DTexture;
    lRet = m_direct3DDevice->CreateTexture(
        width, 
        height, 
        1, 
        NULL,
        D3DFMT_X8R8G8B8,
        D3DPOOL_MANAGED,
        &direct3DTexture,
        NULL);

    if (FAILED(lRet))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create texture.\n");
        return false;
    }
    m_direct3DTexture.reset(direct3DTexture);

    IDirect3DVertexBuffer9 *direct3DVertexBuffer;
    lRet = m_direct3DDevice->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX),
        NULL,
        D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1,
        D3DPOOL_MANAGED,
        &direct3DVertexBuffer,
        NULL);

    if (FAILED(lRet))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create vertex buffer.\n");
        return false;
    }
    m_direct3DVertexBuffer.reset(direct3DVertexBuffer);

    CUSTOMVERTEX vertices[] = {
        { 0.0f,		0.0f,		0.0f,	1.0f,   D3DCOLOR_ARGB(255, 255, 255, 255),0.0f,0.0f },
        { width,	0.0f,		0.0f,	1.0f,   D3DCOLOR_ARGB(255, 255, 255, 255),1.0f,0.0f },
        { width,	height, 	0.0f,	1.0f,   D3DCOLOR_ARGB(255, 255, 255, 255),1.0f,1.0f },
        { 0.0f,		height, 	0.0f,	1.0f,   D3DCOLOR_ARGB(255, 255, 255, 255),0.0f,1.0f }
    };

    CUSTOMVERTEX *pVertex;
    lRet = m_direct3DVertexBuffer->Lock(0, 4 * sizeof(CUSTOMVERTEX), (void**)&pVertex, 0);
    if (FAILED(lRet))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to Lock vertex buffer.\n");
        return false;
    }

    memcpy(pVertex, vertices, sizeof(vertices));
    m_direct3DVertexBuffer->Unlock();

    m_playState.videoDisplay = true;
    return true;
}

bool MediaDisplay_Directx::initVideoSetting(int width, int height, const char *title)
{
    //initialize window
    wchar_t wTitle[1024];
    NarrowToWideBuf(title, wTitle);
    SetWindowText(m_mainWnd, wTitle);

    int x = GetSystemMetrics(SM_CXSCREEN);
    int y = GetSystemMetrics(SM_CYSCREEN);
    x = (x - width) / 2;
    y = (y - height) / 2;
    SetWindowPos(m_mainWnd, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);

    //initialize DirectX 9
    initD3D(width, height);

    return true;
}


bool MediaDisplay_Directx::initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout)
{
    if (!m_audioPlay.isValid())
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create XAudioPlay class.\n");
        return false;
    }

    int channels_;

    if (!wantedChannelLayout || wantedChannels != av_get_channel_layout_nb_channels(wantedChannelLayout))
    {
        wantedChannelLayout = av_get_default_channel_layout(wantedChannels);
        wantedChannelLayout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wantedChannels = av_get_channel_layout_nb_channels(wantedChannelLayout);
    channels_ = wantedChannels;

    if (freq <= 0 || channels_ <= 0)
    {
        msgOutput(MsgType::MSG_ERROR, "Invalid sample rate or channel count!\n");
        return false;
    }
    m_audioParams.fmt = AV_SAMPLE_FMT_S16;
    m_audioParams.freq = freq;
    m_audioParams.channels = channels_;
    m_audioParams.channelLayout = wantedChannelLayout;
    m_audioParams.frameSize = av_samples_get_buffer_size(NULL, m_audioParams.channels, 1, m_audioParams.fmt, 1);
    m_audioParams.bytesPerSec = av_samples_get_buffer_size(NULL, m_audioParams.channels, m_audioParams.freq, m_audioParams.fmt, 1);

    m_audioBufferSamples = FFMAX(audioMinBufferSize, 2 << av_log2(freq / audioMaxCallBackPerSec));

    //create pcm data array
    m_audioBuffer.PCMBufferSize = av_samples_get_buffer_size(NULL, m_audioParams.channels, m_audioBufferSamples, AV_SAMPLE_FMT_S16, 1);
    m_audioBuffer.PCMBuffer = (uint8_t *)av_malloc(m_audioBuffer.PCMBufferSize);

    m_audioPlay.setBufferSize(AUDIO_QUEUE_SIZE, m_audioBuffer.PCMBufferSize);
    if (!m_audioPlay.setFormat(m_audioParams.frameSize, m_audioParams.channels, m_audioParams.freq))
        return false;

    m_playState.audioDisplay = true;
    return true;
}


void MediaDisplay_Directx::exec()
{
    if (m_playState.videoDisplay && m_fps > 0)
    {
        m_playState.delay = 1000 / m_fps;
        m_renderControlThread = std::thread(renderControlThread, this);
    }
    if (m_playState.audioDisplay)
    {
        m_audioPlay.startPlaying();
        m_loadAudioControlThread = std::thread(loadAudioDataThread, this);
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(m_mainWnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case RENDER_NEXT_FRAME:
        MediaDisplay_Directx::renderNextFrame(wparma);
    }
    return DefWindowProc(hwnd, msg, wparma, lparam);
}

void MediaDisplay_Directx::renderControlThread(MediaDisplay_Directx* p)
{
    while (!p->m_playState.exit && !p->m_mainCtrl->isVideoFrameEmpty())
    {
        SendMessage(p->m_mainWnd, RENDER_NEXT_FRAME, WPARAM(p), NULL);
        std::chrono::milliseconds dura(p->m_playState.delay);
        std::this_thread::sleep_for(dura);
    }
}

void MediaDisplay_Directx::loadAudioDataThread(MediaDisplay_Directx* p)
{
    while (!p->m_playState.exit && !p->m_mainCtrl->isAudioFrameEmpty())
    {
        while()
        //p->m_audioPlay.readData(buffer, length);
    }
}

void MediaDisplay_Directx::renderNextFrame(WPARAM wp)
{
    MediaDisplay_Directx *p = reinterpret_cast<MediaDisplay_Directx*>(wp);
    LRESULT lRet;

    if (p->m_videoBuffer.data[0] == nullptr)
    {
        if (p->m_videoBuffer.data[0] != nullptr)
            free(p->m_videoBuffer.data[0]);
        p->m_videoBuffer.size = av_image_get_buffer_size(AV_PIX_FMT_BGRA, p->m_frameWidth, p->m_frameHeight, 1);
        unsigned char* outBuffer = (unsigned char*)av_malloc(p->m_videoBuffer.size);
        av_image_fill_arrays(p->m_videoBuffer.data, p->m_videoBuffer.lineSize, outBuffer, AV_PIX_FMT_BGRA, p->m_frameWidth, p->m_frameHeight, 1);
    }
    if (!p->m_mainCtrl->getGraphicData(GraphicDataType::BGRA, p->m_frameWidth, p->m_frameHeight, p->m_videoBuffer.data, p->m_videoBuffer.size, p->m_videoBuffer.lineSize, &p->m_playState.currentVideoTime))
    {
        p->msgOutput(MsgType::MSG_ERROR, "get rgba data failed\n");
        return;
    }
    //lRet = p->m_direct3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    D3DLOCKED_RECT d3dRect;
    lRet = p->m_direct3DTexture->LockRect(0, &d3dRect, 0, 0);
    if (FAILED(lRet))
    {
        p->msgOutput(MsgType::MSG_ERROR, "Failed to Lock Rect\n");
        return;
    }
    // copy rgb data to texture
    byte *pSrc = p->m_videoBuffer.data[0];
    byte *pDest = (byte *)d3dRect.pBits;
    int stride = d3dRect.Pitch;

    for (unsigned long i = 0; i < p->m_frameHeight; i++)
    {
        memcpy(pDest, pSrc, p->m_videoBuffer.lineSize[0]);
        pDest += stride;
        pSrc += p->m_videoBuffer.lineSize[0];
    }
    p->m_direct3DTexture->UnlockRect(0);

    lRet = p->m_direct3DDevice->BeginScene();
    if (FAILED(lRet))
    {
        p->msgOutput(MsgType::MSG_ERROR, "Failed to begin scene\n");
        return;
    }
    p->m_direct3DDevice->SetTexture(0, p->m_direct3DTexture.get());
    p->m_direct3DDevice->SetStreamSource(0, p->m_direct3DVertexBuffer.get(), 0, sizeof(CUSTOMVERTEX));
    p->m_direct3DDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    p->m_direct3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
    p->m_direct3DDevice->EndScene();
    p->m_direct3DDevice->Present(NULL, NULL, NULL, NULL);
}