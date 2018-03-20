#include "mediaDisply_D3D11.h"
#include "util.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

static LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam);

//typedef HRESULT WINAPI pCreateDeviceManager9(UINT *, IDirect3DDeviceManager9 **);

MediaDisplayD3D11::MediaDisplayD3D11(MediaMainControl *mainCtrl)
    :MediaDisplay(mainCtrl)
{
    m_displayType = DisplayType::USING_D3D9;
    InitializeCriticalSection(&m_critial);
}

MediaDisplayD3D11::~MediaDisplayD3D11()
{
}

bool MediaDisplayD3D11::init()
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

bool MediaDisplayD3D11::initD3D(int width, int height)
{
    //IDXGIDevice * pDXGIDevice = nullptr;
    //hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);

    //IDXGIAdapter * pDXGIAdapter = nullptr;
    //hr = pDXGIDevice->GetAdapter(&pDXGIAdapter);

    //IDXGIFactory * pIDXGIFactory = nullptr;
    //pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);



    return true;
}

bool MediaDisplayD3D11::initVideoSetting(int width, int height, const char *title)
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


bool MediaDisplayD3D11::initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout)
{
    return true;
}


void MediaDisplayD3D11::exec()
{

}

LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam)
{
    return NULL;
}