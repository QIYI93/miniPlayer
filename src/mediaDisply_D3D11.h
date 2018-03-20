#ifndef MediaDisplay_D3D11_H
#define MediaDisplay_D3D11_H

#include <memory>
#include <mutex>
#include <windows.h>
#include <xaudio2.h>
#include <D3D11.h>
#include <DXGI.h>
#include "XAudioPlay.h"

#include "mediaDisplay.h"

class MediaDisplayD3D11 : public MediaDisplay
{
public:
    explicit MediaDisplayD3D11(MediaMainControl *mainCtrl);

    virtual bool init() override;
    virtual bool initVideoSetting(int width, int height, const char *title) override;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout) override;

    virtual void exec() override;

public:
    virtual HWND getWinHandle() { return m_mainWnd; }
private:
    ~MediaDisplayD3D11();
    bool initD3D(int, int);

private:
    HWND m_mainWnd = NULL;
    CRITICAL_SECTION  m_critial;

    IDXGIDevice *m_DXGIDevice = nullptr;
    IDXGIAdapter *m_DXGIAdapter = nullptr;
    IDXGIFactory *m_DXGIFactory = nullptr;
};

#endif