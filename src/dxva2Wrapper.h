#ifndef DXVA2WRAPPER_H
#define DXVA2WRAPPER_H

#include <windows.h>
#include <stdint.h>
#include <d3d9.h>
#include <dxva2api.h>
#include "util/ComPtr.hpp"

extern "C"
{
#include "libavutil/pixfmt.h"
}

enum class HWAccelID : int{
    HWACCEL_NONE = 0,
    HWACCEL_AUTO,
    HWACCEL_VDPAU,
    HWACCEL_DXVA2,
    HWACCEL_VDA,
    HWACCEL_VIDEOTOOLBOX,
    HWACCEL_QSV,
};
class AVCodec;
class AVCodecContext;
class MediaDisplay_D3D9;
class d3d_format_t;
class AVFrame;
class SurfaceInfo;
typedef struct DXVA2Context
{
    LPDIRECT3DSURFACE9  *surfaces;
    SurfaceInfo         *surfaceInfos;
    uint32_t            numSurfaces;
    uint64_t            surfaceAge;
    AVFrame             *tmp_frame;
} DXVA2Context;

typedef struct DXVA2SurfaceWrapper {
    DXVA2Context         *ctx;
    LPDIRECT3DSURFACE9   surface;
    IDirectXVideoDecoder *decoder;
} DXVA2SurfaceWrapper;

class Dxva2Wrapper
{
public:
    explicit Dxva2Wrapper(AVCodec* ,AVCodecContext*, MediaDisplay_D3D9*);
    ~Dxva2Wrapper();
    bool init(HWND);
public:

    static int dxva2GetBuffer(AVCodecContext*, AVFrame*, int);
    static AVPixelFormat GetHwFormat(AVCodecContext*, const AVPixelFormat*);
private:
    bool createDXVA2Decoder();
    void DXVA2DestroyDecoder();
    const d3d_format_t *d3dFindFormat(D3DFORMAT);
    bool DXVA2GetDecoderConfiguration(const GUID *, const DXVA2_VideoDesc *, DXVA2_ConfigPictureDecode *);
private:
    HWAccelID m_hwAccelId = HWAccelID::HWACCEL_AUTO;
    HWAccelID m_activeHwAccelId = HWAccelID::HWACCEL_AUTO;
    AVPixelFormat m_hwAccelPixFmt;
    char * m_hwAccelDevice = "dxva2";

    MediaDisplay_D3D9* m_mediaPlayer = nullptr;
    AVCodec *m_avCodec = nullptr;
    AVCodecContext *m_avCodecCtx = nullptr;

    ComPtr<IDirect3DDeviceManager9> m_deviceManager;
    ComPtr<IDirectXVideoDecoderService>m_decoderService;
    ComPtr<IDirectXVideoDecoder> m_DXVAdecoder;
    GUID m_decodeGuid = GUID_NULL;
    DXVA2_ConfigPictureDecode m_decodeConfig;

    unsigned m_resetToken = 0;
    HANDLE m_deviceHandle;
    DXVA2Context *m_DXVA2Context = nullptr;

};



#endif
