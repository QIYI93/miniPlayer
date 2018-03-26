#include "mediaDisplay_D3D9.h"
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

    m_mainWnd = CreateWindow(L"D3DWindow", L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);
    if (m_mainWnd == NULL)
        return false;

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

    m_d3d9->GetAdapterDisplayMode(NULL, &m_displayMode);
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
    ret = m_d3d9->CreateDevice(NULL, D3DDEVTYPE_HAL, m_mainWnd, behaviorFlags, &m_d3dpp, &m_device);
    if (FAILED(ret))
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create d3d device.");
        return false;
    }

    return true;
}

bool MediaDisplay_D3D9::initVideoSetting(int width, int height, const char *title)
{
    ////initialize window
    //wchar_t wTitle[1024];
    //util::NarrowToWideBuf(title, wTitle);
    //SetWindowText(m_mainWnd, wTitle);

    //int x = GetSystemMetrics(SM_CXSCREEN);
    //int y = GetSystemMetrics(SM_CYSCREEN);
    //x = (x - width) / 2;
    //y = (y - height) / 2;
    //SetWindowPos(m_mainWnd, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);

    ////initialize DirectX 9
    //initD3D(width, height);

    return true;
}


bool MediaDisplay_D3D9::initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout)
{
    return true;
}


void MediaDisplay_D3D9::exec()
{

}

LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case RENDER_NEXT_FRAME:
        break;
    }
    return DefWindowProc(hwnd, msg, wparma, lparam);
}
