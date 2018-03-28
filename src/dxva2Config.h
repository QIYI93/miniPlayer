#include <initguid.h>
extern "C"
{
#include <libavformat/avformat.h>
#include "libavcodec/dxva2.h"
#include "libavutil/avassert.h"
#include "libavutil/buffer.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
}


typedef struct d3d_format_t
{
    const char   *name;
    D3DFORMAT    format;
    AVPixelFormat  codec;
} d3d_format_t;

static const d3d_format_t d3d_formats[] = 
{
    { "YV12", (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), AV_PIX_FMT_YUV420P },
    { "NV12", (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'), AV_PIX_FMT_NV12 },
    { NULL, (D3DFORMAT)0, AV_PIX_FMT_NONE }
};
// referenced from vla
/* define all the GUIDs used directly here,
to avoid problems with inconsistent dxva2api.h versions in mingw-w64 and different MSVC version */

DEFINE_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02);

DEFINE_GUID(DXVA2_ModeMPEG2_VLD, 0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD, 0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID(DXVA2_ModeH264_E, 0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeH264_F, 0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E, 0x604F8E68, 0x4951, 0x4C54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVA2_ModeVC1_D, 0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010, 0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main10, 0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13);
DEFINE_GUID(DXVA2_ModeVP9_VLD_Profile0, 0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);
DEFINE_GUID(DXVA2_NoEncrypt, 0x1b81beD0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(GUID_NULL, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

typedef IDirect3D9* WINAPI pDirect3DCreate9(UINT);
typedef HRESULT WINAPI pCreateDeviceManager9(UINT *, IDirect3DDeviceManager9 **);

CRITICAL_SECTION cs;

typedef struct dxva2_mode {
    const GUID     *guid;
    enum AVCodecID codec;
} dxva2_mode;

static const dxva2_mode dxva2_modes[] = {
    /* MPEG-2 */
    { &DXVA2_ModeMPEG2_VLD, AV_CODEC_ID_MPEG2VIDEO },
    { &DXVA2_ModeMPEG2and1_VLD, AV_CODEC_ID_MPEG2VIDEO },

    /* H.264 */
    { &DXVA2_ModeH264_F, AV_CODEC_ID_H264 },
    { &DXVA2_ModeH264_E, AV_CODEC_ID_H264 },
    /* Intel specific H.264 mode */
    { &DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264 },

    /* VC-1 / WMV3 */
    { &DXVA2_ModeVC1_D2010, AV_CODEC_ID_VC1 },
    { &DXVA2_ModeVC1_D2010, AV_CODEC_ID_WMV3 },
    { &DXVA2_ModeVC1_D, AV_CODEC_ID_VC1 },
    { &DXVA2_ModeVC1_D, AV_CODEC_ID_WMV3 },

    /* HEVC/H.265 */
    { &DXVA2_ModeHEVC_VLD_Main, AV_CODEC_ID_HEVC },
    { &DXVA2_ModeHEVC_VLD_Main10, AV_CODEC_ID_HEVC },

    /* VP8/9 */
    { &DXVA2_ModeVP9_VLD_Profile0, AV_CODEC_ID_VP9 },

    { NULL, AV_CODEC_ID_NONE },
};

typedef struct SurfaceInfo
{
    int used;
    uint64_t age;
} SurfaceInfo;