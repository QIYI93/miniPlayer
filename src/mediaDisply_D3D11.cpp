#include "mediaDisply_D3D11.h"
#include "util.h"
#include <cassert>

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
    m_displayType = DisplayType::USING_D3D11;
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

    if (initD3D())
        return false;
    return true;
}
 
LRESULT WINAPI winProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparma, lparam);
}

bool MediaDisplayD3D11::initD3D()
{
    D3DFactory::CreateFactory();

    m_adapter = D3DFactory::GetFirstAdapter();
    m_ouputs = m_adapter->GetFirstOutput();
  
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);
    HRESULT ret;
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        m_driverType = driverTypes[driverTypeIndex];
        ret = D3D11CreateDevice(
            NULL,
            m_driverType,
            NULL,
            createDeviceFlags,
            NULL,
            NULL,
            D3D11_SDK_VERSION,
            &m_D3D11Device,
            &m_featureLevel,
            &m_D3D11DeviceContext
        );
        if(SUCCEEDED(ret))
            break;
    }
    if (FAILED(ret))
        return false;

    UINT msaaQuality;
    m_D3D11Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality);

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
