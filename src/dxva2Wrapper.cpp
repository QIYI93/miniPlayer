#include "dxva2Wrapper.h"
#include "mediaDisplay.h"
#include "mediaDisplay_D3D9.h"
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

#pragma comment(lib,"Dxva2.lib")


Dxva2Wrapper::Dxva2Wrapper(AVCodec* avCodec,AVCodecContext* avCodecCtx,MediaDisplay_D3D9 *display)
    :m_avCodec(avCodec),
    m_avCodecCtx(avCodecCtx),
    m_mediaPlayer(display)
{
}

Dxva2Wrapper::~Dxva2Wrapper()
{
}

bool Dxva2Wrapper::init(HWND hWnd)
{
    int logLevel = (m_hwAccelId == HWAccelID::HWACCEL_AUTO)? AV_LOG_VERBOSE : AV_LOG_ERROR;
    DXVA2Context *ctx = nullptr;

    HRESULT ret;
    ret = DXVA2CreateDirect3DDeviceManager9(&m_resetToken, &m_deviceManager);
    if (FAILED(ret))
    {
        av_log(NULL, logLevel, "Failed to create Direct3D device manager\n");
        return false;
    }
    ret = m_deviceManager->ResetDevice(m_mediaPlayer->m_device, m_resetToken);
    if (FAILED(ret))
    {
        av_log(NULL, logLevel, "Failed to bind Direct3D device to device manager\n");
        return false;
    }
    ret = m_deviceManager->OpenDeviceHandle(&m_deviceHandle);
    if (FAILED(ret))
    {
        av_log(NULL, logLevel, "Failed to open device handle\n");
        return false;
    }
    ret = m_deviceManager->GetVideoService(m_deviceHandle, IID_PPV_ARGS(&m_decoderService));
    if (FAILED(ret))
    {
        av_log(NULL, logLevel, "Failed to create IDirectXVideoDecoderService\n");
        return false;
    }

    m_dxva2Context.tmp_frame = av_frame_alloc();
    m_avCodecCtx->hwaccel_context = av_mallocz(sizeof(struct dxva_context));
    return true;
}