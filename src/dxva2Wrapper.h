#ifndef DXVA2WRAPPER_H
#define DXVA2WRAPPER_H

#include <windows.h>
#include <stdint.h>
#include <d3d9.h>
#include <dxva2api.h>
#include "util/ComPtr.hpp"

enum class HWAccelID : int{
    HWACCEL_NONE = 0,
    HWACCEL_AUTO,
    HWACCEL_VDPAU,
    HWACCEL_DXVA2,
    HWACCEL_VDA,
    HWACCEL_VIDEOTOOLBOX,
    HWACCEL_QSV,
};

typedef struct surface_info
{
    int used;
    uint64_t age;
} surface_info;

class AVFrame;
typedef struct DXVA2Context
{
    //IDirect3D9                  *d3d9;
    //IDirect3DDevice9            *d3d9device;
    //IDirect3DDeviceManager9     *d3d9devmgr;
    //IDirectXVideoDecoderService *decoder_service;
    //IDirectXVideoDecoder        *decoder;

    //GUID                        decoder_guid;
    //DXVA2_ConfigPictureDecode   decoder_config;

    //LPDIRECT3DSURFACE9          *surfaces;
    //surface_info                *surface_infos;
    //uint32_t                    num_surfaces;
    //uint64_t                    surface_age;

    AVFrame                     *tmp_frame;
} DXVA2Context;

class AVCodec;
class AVCodecContext;
class MediaDisplay_D3D9;
class Dxva2Wrapper
{
public:
    explicit Dxva2Wrapper(AVCodec* ,AVCodecContext*, MediaDisplay_D3D9*);
    ~Dxva2Wrapper();
    bool init(HWND);

private:
    HWAccelID m_hwAccelId = HWAccelID::HWACCEL_AUTO;
    HWAccelID m_activeHwAccelId = HWAccelID::HWACCEL_AUTO;
    char * m_hwAccelDevice = "dxva2";

    MediaDisplay_D3D9* m_mediaPlayer = nullptr;
    AVCodec *m_avCodec = nullptr;
    AVCodecContext *m_avCodecCtx = nullptr;
    void  *m_hwAccelCtx = nullptr;

    ComPtr<IDirect3DDeviceManager9> m_deviceManager;
    ComPtr<IDirectXVideoDecoderService>m_decoderService;
    unsigned m_resetToken = 0;
    HANDLE m_deviceHandle;
    DXVA2Context m_dxva2Context = { 0 };
};



#endif
