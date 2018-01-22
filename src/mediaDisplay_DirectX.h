#ifndef MediaDisplay_Directx_H
#define MediaDisplay_Directx_H

#include <memory>
#include <mutex>
#include "windows.h"
#include "DirectX_SDK/d3d9.h"

#pragma comment (lib,"d3d9.lib")

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

class MediaDisplay_Directx : public MediaDisplay
{
public:
    MediaDisplay_Directx(MediaMainControl *mainCtrl);

    virtual bool init() override;
    virtual bool initVideoSetting(int width, int height, const char *title) override;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout) override;

    virtual void exec() override;

    static void renderNextFrame(WPARAM);
private:
    MediaDisplay_Directx(MediaDisplay_Directx&) = delete;
    ~MediaDisplay_Directx();
    bool initD3D(int, int);
    static void renderControlThread(MediaDisplay_Directx*);


private:
    HWND m_mainWnd = NULL;
    CRITICAL_SECTION  m_critial;
    D3DDISPLAYMODE m_d3dDisplayMode;
    D3DCAPS9 m_d3dcaps;
    int m_supportHardwareVP = 0;;

    std::unique_ptr<IDirect3D9, void(*)(IDirect3D9 *p)> m_direct3D9;
    std::unique_ptr<IDirect3DDevice9, void(*)(IDirect3DDevice9*)> m_direct3DDevice;
    std::unique_ptr<IDirect3DTexture9, void(*)(IDirect3DTexture9*)> m_direct3DTexture;
    std::unique_ptr<IDirect3DVertexBuffer9, void(*)(IDirect3DVertexBuffer9*)> m_direct3DVertexBuffer;

    std::thread m_renderControlThread;

    videoBufferForDirectX m_videoBuffer;
    //AudioBuffer m_audioBuffer;
    //bool m_quit = false;

    int m_frameWidth = 0;
    int m_frameHeight = 0;
};

#endif