#include "dxva2Wrapper.h"

#include <d3d11.h>
//#include <dxva2api.h>

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


Dxva2Wrapper::Dxva2Wrapper(AVCodec* avCodec,AVCodecContext* avCodecCtx)
    :m_avCodec(avCodec),
    m_avCodecCtx(avCodecCtx)
{
}

Dxva2Wrapper::~Dxva2Wrapper()
{
}

bool Dxva2Wrapper::init(HWND hWnd)
{
    int logLevel = (m_hwAccelId == HWAccelID::HWACCEL_AUTO)? AV_LOG_VERBOSE : AV_LOG_ERROR;
    DXVA2Context *ctx = nullptr;
    int ret;
    //if()
    return true;
}