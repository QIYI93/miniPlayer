#include "MediaDisplay_Directx.h"
#include "util.h"


extern "C"
{
//#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
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
    return true;
}


void MediaDisplay_Directx::exec()
{
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
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparma, lparam);
}