#ifndef MediaDisplay_D3D9_H
#define MediaDisplay_D3D9_H

#include <memory>
#include <mutex>
#include <windows.h>
#include <xaudio2.h>
#include <d3d9.h>
#include "util/ComPtr.hpp"
#include "XAudioPlay.h"
#include "dxva2Wrapper.h"
#include "mediaDisplay.h"

typedef struct
{
    FLOAT       x, y, z;
    FLOAT       rhw;
    D3DCOLOR    diffuse;
    FLOAT       tu, tv;
} CUSTOMVERTEX;

typedef struct videoBufferForDirectX
{
    uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
    int  lineSize[AV_NUM_DATA_POINTERS] = { 0 };
    uint32_t size;
}videoBufferForDirectX;

//typedef struct AudioBufferForXAudio
//{
//    uint8_t *PCMBuffer = nullptr;
//    uint8_t *pos = 0;
//    int PCMBufferSize = 0;
//    int restSize = 0;
//    int bytesPerSec = 0;
//}AudioBufferForXAudio;

class MediaDisplay_D3D9 : public MediaDisplay
{
    friend class Dxva2Wrapper;
public:
    MediaDisplay_D3D9(MediaMainControl *mainCtrl);

    virtual bool init() override;
    virtual bool initVideoSetting(int width, int height, const char *title) override;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout) override;

    virtual void exec() override;
    virtual HWND getWinHandle() { return m_mainWnd; }

private:
    MediaDisplay_D3D9(MediaDisplay_D3D9&) = delete;
    ~MediaDisplay_D3D9();

    bool initD3D();

private:
    HWND m_mainWnd = NULL;
    CRITICAL_SECTION  m_critial;
    D3DDISPLAYMODE m_displayMode;
    D3DPRESENT_PARAMETERS m_d3dpp = { 0 };

    ComPtr<IDirect3D9> m_d3d9;
    ComPtr<IDirect3DDevice9> m_device;
};

#endif