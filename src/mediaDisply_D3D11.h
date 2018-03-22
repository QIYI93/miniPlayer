#ifndef MediaDisplay_D3D11_H
#define MediaDisplay_D3D11_H

#include <memory>
#include <mutex>
#include <windows.h>
#include <xaudio2.h>
#include <D3D11.h>
#include <DXGI.h>
#include "DXGI/d3dfactory.h"
#include "DXGI/d3dadapter.h"
#include "DXGI/d3doutput.h"
#include "XAudioPlay.h"
#include "mediaDisplay.h"

#pragma comment(lib,"D3D11.lib")

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
    bool initD3D();

private:
    HWND m_mainWnd = NULL;
    CRITICAL_SECTION  m_critial;
    D3DAdapter *m_adapter = nullptr;
    D3DOutput *m_ouputs = nullptr;
    IDXGIDevice *m_DXGIDevice = nullptr;
    IDXGIAdapter *m_DXGIAdapter = nullptr;
    IDXGIFactory *m_DXGIFactory = nullptr;
    ID3D11Device *m_D3D11Device = nullptr;
    ID3D11DeviceContext *m_D3D11DeviceContext = nullptr;
    D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;
    D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;
};

#endif