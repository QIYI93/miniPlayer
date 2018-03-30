#include "mediaDisplay_D3D9.h"
#include "util.h"
#include <windef.h>
#include <iostream>
extern "C"
{
//#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib,"d3d9.lib")

#define RENDER_NEXT_FRAME (WM_USER + 0x100)
#define CLOSE_WINDOW (WM_USER + 0x101)
#define LOAD_FRAME (WM_USER + 0x102)
#define AUDIO_QUEUE_SIZE 3

namespace
{
    auto const audioMinBufferSize = 512;
    auto const audioMaxCallBackPerSec = 30;
}

static LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam);

MediaDisplay_D3D9::MediaDisplay_D3D9(MediaMainControl *mainCtrl)
    :MediaDisplay(mainCtrl)
{
    m_displayType = DisplayType::USING_D3D9;
    InitializeCriticalSection(&m_critial);
}

MediaDisplay_D3D9::~MediaDisplay_D3D9()
{
}

bool MediaDisplay_D3D9::init()
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = (WNDPROC)winProc;
    wc.lpszClassName = L"D3DWindow";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassEx(&wc);

    m_mainWnd = CreateWindow(L"D3DWindow", L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 960, CW_USEDEFAULT, 480, NULL, NULL, NULL, NULL);
    if (m_mainWnd == NULL)
        return false;
    ShowWindow(m_mainWnd, SW_SHOW);
    UpdateWindow(m_mainWnd);
    if (initD3D())
        return true;
    else
        return false;
}

bool MediaDisplay_D3D9::initD3D()
{
    HRESULT ret;
    m_d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if(m_d3d9 == nullptr)
        return false;
    UINT adapter = D3DADAPTER_DEFAULT;
    m_d3d9->GetAdapterDisplayMode(adapter, &m_displayMode);
    m_d3dpp.Windowed = true;
    m_d3dpp.BackBufferCount = 0;
    m_d3dpp.hDeviceWindow = m_mainWnd;
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.BackBufferFormat = m_displayMode.Format;
    m_d3dpp.EnableAutoDepthStencil = FALSE;
    m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
    m_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    D3DCAPS9 caps;
    DWORD behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
    ret = m_d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    if (SUCCEEDED(ret))
    {
        if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
            behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
        else
            behaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
    }
    ret = m_d3d9->CreateDevice(adapter, D3DDEVTYPE_HAL, m_mainWnd, behaviorFlags, &m_d3dpp, &m_device);
    if (FAILED(ret))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create d3d device.");
        return false;
    }
    return true;
}

bool MediaDisplay_D3D9::initVideoSetting(int width, int height, const char *title)
{
    //initialize window
    wchar_t wTitle[1024];
    util::NarrowToWideBuf(title, wTitle);
    SetWindowText(m_mainWnd, wTitle);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int finalWidth, finalHeight;

    auto getScale = [](int beforeWidth, int beforeHeight, int afterWidth, int afterHeight)->double
    {
        double narrowRateW = 1.0, narrowRateH = 1.0;
        narrowRateW = (double)afterWidth / (double)beforeWidth;
        narrowRateH = (double)afterHeight / (double)beforeHeight;
        return narrowRateW > narrowRateH ? narrowRateW : narrowRateH;
    };

    if (width <= screenWidth && height <= screenHeight)
    {
        finalWidth = width;
        screenHeight = height;
    }
    else
    {
        auto scale = getScale(width, height, screenWidth, screenHeight);
        finalWidth = width * scale;
        finalHeight = height * scale;
    }

    int x = (screenWidth - finalWidth) / 2;
    int y = (screenHeight - finalHeight) / 2;
    //SetWindowPos(m_mainWnd, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
    m_playState.videoDisplay = true;
    return true;
}


bool MediaDisplay_D3D9::initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout)
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

    m_audioBufferSizePerDeliver = av_samples_get_buffer_size(NULL, m_audioParams.channels, m_audioBufferSamples, AV_SAMPLE_FMT_S16, 1);
    m_audioPlay.setBufferSize(AUDIO_QUEUE_SIZE, m_audioBufferSizePerDeliver);
    if (!m_audioPlay.setFormat(av_samples_get_buffer_size(NULL, 1, 1, m_audioParams.fmt, 1), m_audioParams.channels, m_audioParams.freq))
        return false;

    m_playState.audioDisplay = true;
    return true;
}


void MediaDisplay_D3D9::exec()
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

void MediaDisplay_D3D9::renderControlThread(MediaDisplay_D3D9* p)
{
    while (!p->m_playState.exit && !p->m_mainCtrl->isVideoFrameEmpty())
    {
        SendMessage(p->m_mainWnd, RENDER_NEXT_FRAME, WPARAM(p), NULL);
        p->getDelay();
        std::chrono::milliseconds dura(p->m_playState.delay);
        std::this_thread::sleep_for(dura);
    }
}

void MediaDisplay_D3D9::loadAudioDataThread(MediaDisplay_D3D9* p)
{
    int PCMBufferSize = p->m_audioBufferSizePerDeliver * 4;
    uint8_t *PCMBuffer = (uint8_t *)av_malloc(PCMBufferSize);
    uint8_t *pos = PCMBuffer;

    int outLen = 0;
    int restLen = 0;

    while (!p->m_playState.exit && !p->m_mainCtrl->isAudioFrameEmpty())
    {
        while (pos - PCMBuffer < p->m_audioBufferSizePerDeliver)
        {
            if (p->m_mainCtrl->isAudioFrameEmpty())
            {
                p->m_audioPlay.readData(PCMBuffer, pos - PCMBuffer);
                return;
            }
            outLen = 0;
            if (p->m_mainCtrl->getPCMData(pos, PCMBufferSize - (pos - PCMBuffer), p->m_audioParams, &p->m_playState.currentAudioTime, &outLen))
            {
                pos += outLen;
            }
        }
        p->m_audioPlay.readData(PCMBuffer, p->m_audioBufferSizePerDeliver);
        restLen = pos - PCMBuffer - p->m_audioBufferSizePerDeliver;
        memcpy_s(PCMBuffer, p->m_audioBufferSizePerDeliver, PCMBuffer + p->m_audioBufferSizePerDeliver, restLen);
        pos = PCMBuffer + restLen;
    }
    av_free(PCMBuffer);
}

void MediaDisplay_D3D9::getDelay()
{
    int currentAudioTime = m_audioPlay.getDuration(); //more precisely

    static int32_t threshold = (1 / (float)m_fps) * 1000;

    int32_t 		retDelay = 0.0;
    int32_t 		compare = 0.0;


    if (m_playState.currentVideoTime == 0 && currentAudioTime == 0)
    {
        m_playState.delay = m_fps;
        return;
    }

    compare = m_playState.currentVideoTime - currentAudioTime;

    if (compare <= -threshold)
        retDelay = m_fps / 2;
    else if (compare >= threshold)
        retDelay = m_fps * 2;
    else
        retDelay = threshold;

    m_playState.delay = retDelay;

    if (retDelay == threshold)
        av_log(nullptr, AV_LOG_INFO, "video_time:%d,audio_time:%d,compare:%d,OK\n", m_playState.currentVideoTime, currentAudioTime, compare);
    else if (retDelay < threshold)
        av_log(nullptr, AV_LOG_INFO, "video_time:%d,audio_time:%d,compare:%d,video delay---------\n", m_playState.currentVideoTime, currentAudioTime, compare);
    else
        av_log(nullptr, AV_LOG_INFO, "video_time:%d,audio_time:%d,compare:%d,video fast----------\n", m_playState.currentVideoTime, currentAudioTime, compare);
}

void MediaDisplay_D3D9::renderNextFrame(WPARAM wp)
{
    MediaDisplay_D3D9 *p = reinterpret_cast<MediaDisplay_D3D9*>(wp);
    LRESULT lRet;
    RECT rect;

    IDirect3DSurface9 *surface = nullptr;
    IDirect3DSurface9 *backBuffer = nullptr;

    EnterCriticalSection(&p->m_critial);

    surface = p->m_mainCtrl->getSurface(&p->m_playState.currentVideoTime);
    if (!surface)
    {
        p->msgOutput(MsgType::MSG_ERROR, "get surface point failed\n");
        return;
    }

    p->m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    p->m_device->BeginScene();
    p->m_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    GetClientRect(p->m_mainWnd, &rect);
    p->m_device->StretchRect(surface, NULL, backBuffer, &rect, D3DTEXF_LINEAR);
    p->m_device->EndScene();
    p->m_device->Present(NULL, NULL, NULL, NULL);
    backBuffer->Release();

    LeaveCriticalSection(&p->m_critial);
}

LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case RENDER_NEXT_FRAME:
        MediaDisplay_D3D9::renderNextFrame(wparma);
        break;
    }
    return DefWindowProc(hwnd, msg, wparma, lparam);
}
