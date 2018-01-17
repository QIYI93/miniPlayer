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
{
    m_displayType = DisplayType::USING_DIRECTX;
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


bool MediaDisplay_Directx::initVideoSetting(int width, int height, const char *title)
{
    wchar_t wTitle[1024];
    NarrowToWideBuf(title, wTitle);
    SetWindowText(m_mainWnd, wTitle);

    int x = GetSystemMetrics(SM_CXSCREEN);
    int y = GetSystemMetrics(SM_CYSCREEN);
    x = (x - width) / 2;
    y = (y - height) / 2;
    SetWindowPos(m_mainWnd, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
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