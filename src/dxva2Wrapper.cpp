#include "dxva2Wrapper.h"
#include "mediaDisplay.h"
#include "mediaDisplay_D3D9.h"
#include "dxva2Config.h"
extern "C"
{
#include <libavformat/avformat.h>
#include "libavcodec/dxva2.h"
#include "libavutil/avassert.h"
#include "libavutil/buffer.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
}

#pragma comment(lib,"Dxva2.lib")

Dxva2Wrapper::Dxva2Wrapper(AVCodec* avCodec,AVCodecContext* avCodecCtx,MediaDisplay_D3D9 *display)
    :m_avCodec(avCodec),
    m_avCodecCtx(avCodecCtx),
    m_mediaPlayer(display)
{
}

Dxva2Wrapper::~Dxva2Wrapper()
{
    DXVA2DestroyDecoder();

    if (m_deviceManager && m_deviceManager != INVALID_HANDLE_VALUE)
        m_deviceManager->CloseDeviceHandle(m_deviceHandle);

    if (m_DXVA2Context)
    {
        av_frame_free(&m_DXVA2Context->tmp_frame);
        av_free(m_DXVA2Context);
    }

}

static void dxva2ReleaseBuffer(void *opaque, uint8_t *data)
{
    DXVA2SurfaceWrapper *surfaceWrapper = (DXVA2SurfaceWrapper *)opaque;
    DXVA2Context *dxvaCtx = surfaceWrapper->ctx;
    for (int i = 0; i < dxvaCtx->numSurfaces; i++)
    {
        if (dxvaCtx->surfaces[i] == surfaceWrapper->surface)
        {
            dxvaCtx->surfaceInfos[i].used = 0;
            break;
        }
    }
    surfaceWrapper->decoder->Release();
    av_free(surfaceWrapper);
}

AVPixelFormat Dxva2Wrapper::GetHwFormat(AVCodecContext *codecContext, const AVPixelFormat *pixFmt)
{
    Dxva2Wrapper *dxva2Wrapper = static_cast<Dxva2Wrapper*>(codecContext->opaque);
    dxva2Wrapper->m_activeHwAccelId = HWAccelID::HWACCEL_DXVA2;
    dxva2Wrapper->m_hwAccelPixFmt = AV_PIX_FMT_DXVA2_VLD;
    return dxva2Wrapper->m_hwAccelPixFmt;
}

int Dxva2Wrapper::dxva2GetBuffer(AVCodecContext *codecContext, AVFrame *frame, int flags)
{
    Dxva2Wrapper *dxva2Wrapper = static_cast<Dxva2Wrapper*>(codecContext->opaque);
    int i;
    int oldUnused = -1;
    LPDIRECT3DSURFACE9 surface;
    DXVA2SurfaceWrapper *surfaceWrapper = nullptr;

    av_assert0(frame->format == AV_PIX_FMT_DXVA2_VLD);

    for (int i = 0; i < dxva2Wrapper->m_DXVA2Context->numSurfaces; i++)
    {
        SurfaceInfo *info = &dxva2Wrapper->m_DXVA2Context->surfaceInfos[i];
        if (!info->used && (oldUnused == -1 || info->age < dxva2Wrapper->m_DXVA2Context->surfaceInfos[oldUnused].age))
            oldUnused = i;
    }
    if (oldUnused == -1)
    {
        av_log(NULL, AV_LOG_ERROR, "No free DXVA2 surface!\n");
        return AVERROR(ENOMEM);
    }
    i = oldUnused;
    surface = dxva2Wrapper->m_DXVA2Context->surfaces[i];
    surfaceWrapper = (DXVA2SurfaceWrapper*)av_mallocz(sizeof(DXVA2SurfaceWrapper));

    frame->buf[0] = av_buffer_create((uint8_t*)surface, 0, dxva2ReleaseBuffer, surfaceWrapper, AV_BUFFER_FLAG_READONLY);
    if (!frame->buf[0])
    {
        av_free(surfaceWrapper);
        return AVERROR(ENOMEM);
    }

    surfaceWrapper->ctx = dxva2Wrapper->m_DXVA2Context;
    surfaceWrapper->surface = surface;
    surfaceWrapper->surface->AddRef();
    surfaceWrapper->decoder = dxva2Wrapper->m_DXVAdecoder;
    surfaceWrapper->decoder->AddRef();

    dxva2Wrapper->m_DXVA2Context->surfaceInfos[i].used = 1;
    dxva2Wrapper->m_DXVA2Context->surfaceInfos[i].age = dxva2Wrapper->m_DXVA2Context->surfaceAge++;
    frame->data[3] = (uint8_t *)surface;

    return 0;
}

bool Dxva2Wrapper::init(HWND hWnd)
{
    DXVA2Context *ctx = nullptr;

    HRESULT ret;
    ret = DXVA2CreateDirect3DDeviceManager9(&m_resetToken, &m_deviceManager);
    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to create Direct3D device manager\n");
        return false;
    }
    ret = m_deviceManager->ResetDevice(m_mediaPlayer->m_device, m_resetToken);
    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to bind Direct3D device to device manager\n");
        return false;
    }
    ret = m_deviceManager->OpenDeviceHandle(&m_deviceHandle);
    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to open device handle\n");
        return false;
    }
    ret = m_deviceManager->GetVideoService(m_deviceHandle, IID_IDirectXVideoDecoderService, (void **)&m_decoderService);
    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to create IDirectXVideoDecoderService\n");
        return false;
    }

    m_DXVA2Context = (DXVA2Context *)av_mallocz(sizeof(*ctx));

    m_DXVA2Context->tmp_frame = av_frame_alloc();
    m_avCodecCtx->hwaccel_context = av_mallocz(sizeof(struct dxva_context));

    if (m_avCodecCtx->codec_id == AV_CODEC_ID_H264 && (m_avCodecCtx->profile & ~FF_PROFILE_H264_CONSTRAINED) > FF_PROFILE_H264_HIGH)
    {
        av_log(NULL, AV_LOG_VERBOSE, "Unsupported H.264 profile for DXVA2 HWAccel: %d\n", m_avCodecCtx->profile);
        return false;
    }

    if (!createDXVA2Decoder())
    {
        av_log(NULL, AV_LOG_VERBOSE, "Error creating the DXVA2 decoder\n");
        DXVA2DestroyDecoder();
        return false;
    }

    return true;
}

bool Dxva2Wrapper::createDXVA2Decoder()
{
    struct dxva_context *dxvaCtx = (dxva_context *)m_avCodecCtx->hwaccel_context; 
    GUID *guidList = NULL;
    unsigned int guidCount = 0;
    unsigned int i, j;
    GUID deviceGuid = GUID_NULL;
    D3DFORMAT targetFormat = D3DFMT_UNKNOWN;
    DXVA2_VideoDesc desc = { 0 };
    DXVA2_ConfigPictureDecode config;
    HRESULT ret;
    int surfaceAlignment;

    ret = m_decoderService->GetDecoderDeviceGuids(&guidCount, &guidList);
    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to retrieve decoder device GUIDs\n");
        return false;
    }

    for (i = 0; dxva2_modes[i].guid; i++)
    {
        D3DFORMAT *targetList = nullptr;
        unsigned int targetCount = 0;
        const dxva2_mode *mode = &dxva2_modes[i];
        if(mode->codec != m_avCodecCtx->codec_id)
            continue;

        for (j = 0; j < guidCount; j++)
        {
            if(IsEqualGUID(*mode->guid,guidList[j]))
                break;
        }
        if(j == guidCount)
            continue;

        ret = m_decoderService->GetDecoderRenderTargets(*mode->guid, &targetCount, &targetList); //枚举支持的渲染格式
        if(FAILED(ret))
            continue;
        //for (unsigned int j = 0; j < targetCount; j++)
        //{
        //    const D3DFORMAT format = targetList[j];
        //    const d3d_format_t *format_t = d3dFindFormat(format);
        //}
        for (unsigned int j = 0; j < targetCount; j++)
        {
            const D3DFORMAT format = targetList[j];
            if (format == MKTAG('N', 'V', '1', '2'))
            {
                targetFormat = format;
                break;
            }
        }
        CoTaskMemFree(targetList);
        if (targetFormat)
        {
            deviceGuid = *mode->guid;
            break;
        }
    }

    CoTaskMemFree(guidList);

    if (IsEqualGUID(deviceGuid, GUID_NULL))
    {
        av_log(NULL, AV_LOG_VERBOSE, "No decoder device for codec found\n");
        return false;
    }

    desc.SampleWidth = m_avCodecCtx->coded_width;
    desc.SampleHeight = m_avCodecCtx->coded_height;
    desc.Format = targetFormat;

    if (!DXVA2GetDecoderConfiguration(&deviceGuid, &desc, &config))
    {
        return false;
    }

    /* decoding MPEG-2 requires additional alignment on some Intel GPUs,
    but it causes issues for H.264 on certain AMD GPUs..... */
    if (m_avCodecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
        surfaceAlignment = 32; 
    /* the HEVC DXVA2 spec asks for 128 pixel aligned surfaces to ensure
    all coding features have enough room to work with */
    else if (m_avCodecCtx->codec_id == AV_CODEC_ID_HEVC)
        surfaceAlignment = 128;
    else
        surfaceAlignment = 16;

    /* 4 base work surfaces */
    m_DXVA2Context->numSurfaces = 4;

    /* add surfaces based on number of possible refs */
    if (m_avCodecCtx->codec_id == AV_CODEC_ID_H264 || m_avCodecCtx->codec_id == AV_CODEC_ID_HEVC)
        m_DXVA2Context->numSurfaces += 16;
    else if (m_avCodecCtx->codec_id == AV_CODEC_ID_VP9)
        m_DXVA2Context->numSurfaces += 8;
    else
        m_DXVA2Context->numSurfaces += 2;

    /* add extra surfaces for frame threading */
    if (m_avCodecCtx->active_thread_type & FF_THREAD_FRAME)
        m_DXVA2Context->numSurfaces += m_avCodecCtx->thread_count;

    m_DXVA2Context->surfaces = (LPDIRECT3DSURFACE9*)av_mallocz(m_DXVA2Context->numSurfaces * sizeof(*m_DXVA2Context->surfaces));
    m_DXVA2Context->surfaceInfos = (SurfaceInfo*)av_mallocz(m_DXVA2Context->numSurfaces * sizeof(*m_DXVA2Context->surfaceInfos));
    
    ret = m_decoderService->CreateSurface(
        FFALIGN(m_avCodecCtx->coded_width, surfaceAlignment),
        FFALIGN(m_avCodecCtx->coded_height, surfaceAlignment),
        m_DXVA2Context->numSurfaces - 1,
        targetFormat, D3DPOOL_DEFAULT, NULL,
        DXVA2_VideoDecoderRenderTarget,
        m_DXVA2Context->surfaces, NULL);

    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to create %d video surfaces\n", m_DXVA2Context->numSurfaces);
        return false;
    }

    ret = m_decoderService->CreateVideoDecoder(deviceGuid, &desc, &config, m_DXVA2Context->surfaces, m_DXVA2Context->numSurfaces, &m_DXVAdecoder);
    if (FAILED(ret))
    {
        av_log(NULL, AV_LOG_VERBOSE, "Failed to create DXVA2 video decoder\n");
        return false;
    }

    m_decodeGuid = deviceGuid;
    m_decodeConfig = config;

    dxvaCtx->cfg = &m_decodeConfig;
    dxvaCtx->decoder = m_DXVAdecoder;
    dxvaCtx->surface = m_DXVA2Context->surfaces;
    dxvaCtx->surface_count = m_DXVA2Context->numSurfaces;

    if (IsEqualGUID(m_decodeGuid, DXVADDI_Intel_ModeH264_E))
        dxvaCtx->workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;

    return true;
}

bool Dxva2Wrapper::DXVA2GetDecoderConfiguration(const GUID *device_guid, const DXVA2_VideoDesc *desc, DXVA2_ConfigPictureDecode *config)
{
    int logLevel = (m_hwAccelId == HWAccelID::HWACCEL_AUTO) ? AV_LOG_VERBOSE : AV_LOG_ERROR;
    unsigned int configCount = 0;
    unsigned int bestScore = 0;
    DXVA2_ConfigPictureDecode *configList = nullptr;
    DXVA2_ConfigPictureDecode bestconfig = { 0 };
    HRESULT ret;

    ret = m_decoderService->GetDecoderConfigurations(*device_guid, desc, NULL, &configCount, &configList);
    if (FAILED(ret))
    {
        av_log(NULL, logLevel, "Unable to retrieve decoder configurations\n");
        return false;
    }
    for (unsigned int i = 0; i < configCount; i++)
    {
        DXVA2_ConfigPictureDecode *cfg = &configList[i];
        unsigned int score;
        if (cfg->ConfigBitstreamRaw == 1)
            score = 1;
        else if (m_avCodecCtx->codec_id == AV_CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2)
            score = 2;
        else
            continue;
        if (IsEqualGUID(cfg->guidConfigBitstreamEncryption, DXVA2_NoEncrypt))
            score += 16;
        if (score > bestScore)
        {
            bestScore = score;
            bestconfig = *cfg;
        }
    }
    CoTaskMemFree(configList);

    if (!bestScore)
    {
        av_log(NULL, logLevel, "No valid decoder configuration available\n");
        return false;
    }
    *config = bestconfig;
    return true;
}

void Dxva2Wrapper::DXVA2DestroyDecoder()
{
    if (m_DXVA2Context != nullptr)
    {
        if (m_DXVA2Context->surfaces)
        {
            for (int i = 0; i < m_DXVA2Context->numSurfaces; i++)
            {
                if (m_DXVA2Context->surfaces[i])
                    IDirect3DSurface9_Release(m_DXVA2Context->surfaces[i]);
            }
        }
        av_freep(&m_DXVA2Context->surfaces);
        av_freep(&m_DXVA2Context->surfaceInfos);
        m_DXVA2Context->numSurfaces = 0;
        m_DXVA2Context->surfaceAge = 0;
    }
    if (m_DXVAdecoder)
    {
        m_DXVAdecoder.Clear();
    }
}

const d3d_format_t *Dxva2Wrapper::d3dFindFormat(D3DFORMAT format)
{
    for (unsigned i = 0; d3d_formats[i].name; i++)
    {
        if (d3d_formats[i].format == format)
            return &d3d_formats[i];
    }
    return NULL;
}