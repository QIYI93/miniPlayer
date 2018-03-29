#include "mediaMainControl.h"

#include <iostream>
#include <windows.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_qsv.h>
#include <libswresample/swresample.h>
}

#include "util.h"
#include "dxva2Wrapper.h"
#include "mediaDisplay.h"
#include "mediaDisplay_D3D9.h"

namespace
{
    auto const errMsgBufferSize = 2048;
    auto const maxAudioFrameSize = 192000;
    auto const maxAudioPktQueueSize = 50;
    auto const maxVideoPktQueueSize = 20;
    auto const maxAudioFrameQueueSize = 100;
    auto const maxVideoFrameQueueSize = 15;

    auto const waitTime = 5;
}

//MediaMainControl* MediaMainControl::getInstance()
//{
//    static MediaMainControl mediaMainControl;
//    return &mediaMainControl;
//}

MediaMainControl::MediaMainControl()
    :m_audioFrameRaw(av_frame_alloc())
    ,m_videoFrameRaw(av_frame_alloc())
{
    av_register_all();
    avformat_network_init();
    m_errMsgBuffer = (char*)malloc(errMsgBufferSize * sizeof(char));
}

MediaMainControl::~MediaMainControl()
{
    free(m_errMsgBuffer);

    av_frame_free(&m_audioFrameRaw);
    av_frame_free(&m_videoFrameRaw);
    closeFile();
}

bool MediaMainControl::openFile(const char *file)
{
    closeFile();
    m_formatCtx = avformat_alloc_context();
    int ret = avformat_open_input(&m_formatCtx, file, nullptr, nullptr);
    if (ret != 0)
    {
        av_strerror(ret, m_errMsgBuffer, errMsgBufferSize);
        av_log(nullptr, AV_LOG_ERROR, m_errMsgBuffer);
        return false;
    }
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        av_log(nullptr, AV_LOG_ERROR, "Could not find codec parameters.");
        return false;
    }

    av_dump_format(m_formatCtx, 0, file, 0);
    if (m_formatCtx->duration != AV_NOPTS_VALUE)
        m_totalTimeMS = (m_formatCtx->duration / AV_TIME_BASE) * 1000;

    //Find video and audio stream
    m_videoStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, NULL);
    m_audioStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, NULL);

    //Open video and audio codec and create codec context
    ret = true;
    if(m_videoStreamIndex >= 0)
        ret &= setVideoDecoder();

    if (m_audioStreamIndex >= 0)
        ret &= setAudioDecoder();

    if (ret != TRUE)
        return false;

    m_file = file;
    return true;
}

bool MediaMainControl::setAudioDecoder()
{
    m_audioCodec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    int ret;
    if (m_audioCodec == nullptr)
    {
        av_log(nullptr, AV_LOG_ERROR, "Not found audio decoder.");
        return false;
    }

    m_audioCodecCtx = avcodec_alloc_context3(m_audioCodec);
    avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioStreamIndex]->codecpar);
    ret = avcodec_open2(m_audioCodecCtx, m_audioCodec, nullptr);
    if (ret != 0)
    {
        av_strerror(ret, m_errMsgBuffer, errMsgBufferSize);
        av_log(nullptr, AV_LOG_ERROR, m_errMsgBuffer);
        return false;
    }
    return true;
}
bool MediaMainControl::setVideoDecoder()
{
    m_videoCodec = avcodec_find_decoder(m_formatCtx->streams[m_videoStreamIndex]->codecpar->codec_id);

    if (m_videoCodec == nullptr)
    {
        av_log(nullptr, AV_LOG_ERROR, "Not found video decoder.");
        return false;
    }
    m_mediaDisplay = MediaDisplay::createDisplayInstance(this, DisplayType::USING_D3D9); //create window
    if (m_mediaDisplay == nullptr)
        return false;
    if (m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate.den != NULL && m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate.num != NULL)
        m_fps = av_q2d(m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate);
    //m_videoCodecCtx = m_formatCtx->streams[m_videoStreamIndex]->codec;
    m_videoCodecCtx = avcodec_alloc_context3(m_videoCodec);
    avcodec_parameters_to_context(m_videoCodecCtx, m_formatCtx->streams[m_videoStreamIndex]->codecpar);
    m_frameWidth = m_videoCodecCtx->width;
    m_frameHeight = m_videoCodecCtx->height;
    AVCodecContext *tempCodecCtx = m_videoCodecCtx;
    memcpy(tempCodecCtx, m_videoCodecCtx, sizeof(m_videoCodecCtx));
    switch (m_videoCodec->id)
    {
    case AV_CODEC_ID_MPEG2VIDEO:
    case AV_CODEC_ID_H264:
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
    case AV_CODEC_ID_HEVC:
    case AV_CODEC_ID_VP9:
    {
        m_videoCodecCtx->thread_count = 1;
        Dxva2Wrapper *m_dxva2Wrapper = new Dxva2Wrapper(m_videoCodec, m_videoCodecCtx, dynamic_cast<MediaDisplay_D3D9*>(m_mediaDisplay));
        m_videoCodecCtx->coded_width = m_videoCodecCtx->width;
        m_videoCodecCtx->coded_height = m_videoCodecCtx->height;
        if (m_dxva2Wrapper->init(m_mediaDisplay->getWinHandle()))
        {
            m_videoCodecCtx->opaque = m_dxva2Wrapper;
            m_videoCodecCtx->get_buffer2 = Dxva2Wrapper::dxva2GetBuffer;
            m_videoCodecCtx->get_format = Dxva2Wrapper::GetHwFormat;
            m_videoCodecCtx->thread_safe_callbacks = 1;
            break;
        }
        else
        {
            delete m_dxva2Wrapper;
            m_isAccelSupport = false;
            break;
        }
    }
    default:
    {
        m_isAccelSupport = false;
        break;
    }
    }
    if (m_isAccelSupport == false)
    {
        av_free(m_videoCodecCtx);
        m_videoCodecCtx = tempCodecCtx;
        m_videoCodecCtx->thread_count = util::getNoOfProcessors();
        m_videoCodecCtx->thread_type = FF_THREAD_FRAME;
        if (m_videoCodecCtx->codec->capabilities & CODEC_CAP_TRUNCATED)
            m_videoCodecCtx->flags |= CODEC_FLAG_TRUNCATED;
    }

    int ret = avcodec_open2(m_videoCodecCtx, m_videoCodec, nullptr);
    if (ret != 0)
    {
        av_strerror(ret, m_errMsgBuffer, errMsgBufferSize);
        av_log(nullptr, AV_LOG_ERROR, m_errMsgBuffer);
        return false;
    }
    return true;
}

void MediaMainControl::closeFile()
{
    m_isQuit = true;

    if (m_videoPktQueue && m_audioPktQueue)
    {
        m_videoPktQueue->clean();
        m_audioPktQueue->clean();
        m_separatePktThread.join();
    }

    if (m_videoFrameQueue && m_audioFrameQueue)
    {
        m_videoFrameQueue->clean();
        m_audioFrameQueue->clean();
        m_decodeAudioThread.join();
        m_decodeVideoThread.join();
    }

    cleanPktQueue();
    cleanFrameQueue();

    avcodec_close(m_videoCodecCtx);
    avcodec_close(m_audioCodecCtx);
    avcodec_free_context(&m_videoCodecCtx);
    avcodec_free_context(&m_audioCodecCtx);

    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }

    if (m_swrCtx)
        swr_free(&m_swrCtx);

    if (m_formatCtx)
        avformat_close_input(&m_formatCtx);
}

void MediaMainControl::initSeparatePktThread(void *mainCtrl)
{    
    auto separatePktFun = [](void *d)
    {
        MediaMainControl *mainCtl = static_cast<MediaMainControl*>(d);
        AVPacket *pAVPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
        //int audioPktCount = 0;
        //int videoPktCount = 0;
        while (true)
        {
            if (!mainCtl->m_formatCtx) break;
            if (mainCtl->m_isQuit)
            {
                mainCtl->m_noPktToSperate = true;
                break;
            }
            int ret = av_read_frame(mainCtl->m_formatCtx, pAVPacket);
            if (ret != NULL)
            {
                if (ret == AVERROR_EOF)
                {
                    mainCtl->m_noPktToSperate = true;
                    break;
                }
                av_strerror(ret, mainCtl->m_errMsgBuffer, errMsgBufferSize);
                av_log(nullptr, AV_LOG_INFO, mainCtl->m_errMsgBuffer);
                continue;
            }
            //enqueue packet
            if (pAVPacket->stream_index == mainCtl->m_audioStreamIndex)
            {
                mainCtl->m_audioPktQueue->enQueue(pAVPacket);
                av_packet_unref(pAVPacket);
            }
            else if (pAVPacket->stream_index == mainCtl->m_videoStreamIndex)
            {
                mainCtl->m_videoPktQueue->enQueue(pAVPacket);
                av_packet_unref(pAVPacket);
            }
            else
                av_packet_unref(pAVPacket);
        }
        av_packet_free(&pAVPacket);
    };

    MediaMainControl *mainCtl = static_cast<MediaMainControl*>(mainCtrl);
    mainCtl->m_separatePktThread = std::thread(separatePktFun, mainCtrl);
}

void MediaMainControl::initDecodePktThread(void *mainCtrl)
{
    auto decodeVideoPktFun = [&](void *d)
    {
        MediaMainControl *mainCtl = static_cast<MediaMainControl*>(d);
        AVFrame *frame = av_frame_alloc();
        AVPacket *videoPacket = av_packet_alloc();
        while (true)
        {
            if (!mainCtl->m_formatCtx) break;
            if (mainCtl->m_isQuit) break;

            while (mainCtl->m_videoPktQueue->m_queue.empty() && !mainCtl->m_noPktToSperate)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
            }
            if (mainCtl->m_videoPktQueue->m_queue.empty())
            {
                mainCtl->m_videoFrameQueue->m_noMorePktToDecode.store(true);
                break;
            }
            //decode video pkt
            mainCtl->m_videoPktQueue->deQueue(videoPacket);
            if (avcodec_send_packet(mainCtl->m_videoCodecCtx, videoPacket) != AVERROR(EAGAIN))
            {
                if (avcodec_receive_frame(mainCtl->m_videoCodecCtx, frame) >= NULL)
                {
                    mainCtl->m_videoFrameQueue->enQueue(frame);
                    av_frame_unref(frame);
                }
            }
            av_packet_unref(videoPacket);
        }
        av_packet_free(&videoPacket);
        av_frame_free(&frame);
    };

    auto decodeAudioPktFun = [&](void *d)
    {
        MediaMainControl *mainCtl = static_cast<MediaMainControl*>(d);
        AVFrame *frame = av_frame_alloc();
        AVPacket *audioPacket = av_packet_alloc();
        while (true)
        {
            if (!mainCtl->m_formatCtx) break;
            if (mainCtl->m_isQuit) break;
            while (mainCtl->m_audioPktQueue->m_queue.empty() && !mainCtl->m_noPktToSperate)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
            }
            if (mainCtl->m_audioPktQueue->m_queue.empty())
            {
                mainCtl->m_audioFrameQueue->m_noMorePktToDecode.store(true);
                break;
            }
            //decode audio pkt
            mainCtl->m_audioPktQueue->deQueue(audioPacket);
            if (avcodec_send_packet(mainCtl->m_audioCodecCtx, audioPacket) == NULL)
            {
                while (avcodec_receive_frame(mainCtl->m_audioCodecCtx, frame) == NULL)
                {
                    //mainCtl->m_audioFrameQueue->enQueue(frame);
                    av_frame_unref(frame);
                }
            }
            av_packet_unref(audioPacket);
        }
        av_packet_free(&audioPacket);
        av_frame_free(&frame);
    };

    MediaMainControl *mainCtl = static_cast<MediaMainControl*>(mainCtrl);
    mainCtl->m_decodeAudioThread = std::thread(decodeAudioPktFun, mainCtrl);
    mainCtl->m_decodeVideoThread = std::thread(decodeVideoPktFun, mainCtrl);
}

IDirect3DSurface9* MediaMainControl::getSurface(int32_t *pts)
{
    av_frame_unref(m_videoFrameRaw);
    IDirect3DSurface9 *surface = nullptr;
    while (!m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    if (m_videoFrameQueue->m_queue.empty())
        return false;

    m_videoFrameQueue->deQueue(m_videoFrameRaw);
    surface = (LPDIRECT3DSURFACE9)m_videoFrameRaw->data[3];
    *pts = getVideoFramPts(m_videoFrameRaw);

    return surface;
}

bool MediaMainControl::getGraphicData(GraphicDataType type, int width, int height, void *data, const uint32_t size, int *lineSize, int32_t *pts)
{
    switch (type)
    {
    case GraphicDataType::YUV420:
        m_swsCtx = sws_getCachedContext(m_swsCtx,
            m_videoCodecCtx->width, m_videoCodecCtx->height,
            m_videoCodecCtx->pix_fmt,
            width, height,
            AV_PIX_FMT_YUV420P,
            SWS_BICUBIC,
            nullptr, nullptr, nullptr);
        break;
    case GraphicDataType::BGRA:
        m_swsCtx = sws_getCachedContext(m_swsCtx,
            m_videoCodecCtx->width, m_videoCodecCtx->height,
            m_videoCodecCtx->pix_fmt,
            width, height,
            AV_PIX_FMT_BGRA,
            SWS_BICUBIC,
            nullptr, nullptr, nullptr);
        break;
    default:
        break;
    }

    if (!m_swsCtx)
    {
        av_log(nullptr, AV_LOG_ERROR, "sws_getCachedContext failed.");
        return false;
    }

    while (!m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    if (m_videoFrameQueue->m_queue.empty())
        return false;

    m_videoFrameQueue->deQueue(m_videoFrameRaw);
    int ret = sws_scale(m_swsCtx, (const uint8_t* const*)m_videoFrameRaw->data, m_videoFrameRaw->linesize, 0, m_videoCodecCtx->height, (uint8_t *const*)data, lineSize);
    *pts = getVideoFramPts(m_videoFrameRaw);
    av_frame_unref(m_videoFrameRaw);

    if (ret > 0)
        return true;
    else
        return false;
}

int32_t MediaMainControl::getVideoFramPts(AVFrame *pframe)
{
    int32_t pts = 0.0;
    double frame_delay = 0.0;

    pts = av_frame_get_best_effort_timestamp(pframe);
    if (pts == AV_NOPTS_VALUE)
        pts = 0;

    pts = pts * av_q2d(m_formatCtx->streams[m_videoStreamIndex]->time_base) * 1000;
    if (pts != 0)
        m_videoClock = pts;
    else
        pts = m_videoClock;

    frame_delay = av_q2d(m_formatCtx->streams[m_videoStreamIndex]->time_base);
    frame_delay += pframe->repeat_pict / (frame_delay * 2);
    m_videoClock += frame_delay * 1000;

    return pts;
}

int32_t MediaMainControl::getAudioFramPts(AVFrame *pframe)
{
    int32_t pts = 0.0;
    double frame_delay = 0.0;

    pts = av_frame_get_best_effort_timestamp(pframe);
    if (pts == AV_NOPTS_VALUE)
        pts = 0;
    pts = pts * av_q2d(m_formatCtx->streams[m_audioStreamIndex]->time_base) * 1000;
    if (pts != 0)
        m_audioClock = pts;
    else
        pts = m_audioClock;

    frame_delay = (double)pframe->nb_samples / (double)m_audioCodecCtx->sample_rate;
    m_audioClock += frame_delay * 1000;
    return pts;

}

bool MediaMainControl::getPCMData(void *data, const uint32_t inLen, const AudioParams para, int32_t *pts, int32_t *outLen)
{
    if (!data || !inLen)
        return false;
    if (m_swrCtx == nullptr)
        m_swrCtx = swr_alloc();

    swr_alloc_set_opts(m_swrCtx,
        para.channelLayout,
        para.fmt,
        para.freq,
        m_audioCodecCtx->channel_layout,
        m_audioCodecCtx->sample_fmt,
        m_audioCodecCtx->sample_rate,
        NULL, NULL);

    if (!m_swrCtx || swr_init(m_swrCtx) < 0)
    {
        av_log(NULL, AV_LOG_ERROR,
            "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
            m_audioCodecCtx->sample_rate, av_get_sample_fmt_name(m_audioCodecCtx->sample_fmt), m_audioCodecCtx->channels,
            para.freq, av_get_sample_fmt_name(para.fmt), para.channels);
        swr_free(&m_swrCtx);
        return false;
    }

    while (!m_audioFrameQueue->m_noMorePktToDecode && m_audioFrameQueue->m_queue.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    if (m_audioFrameQueue->m_queue.empty())
        return false;

    m_audioFrameQueue->deQueue(m_audioFrameRaw);
    int ret = swr_convert(m_swrCtx, (uint8_t**)&data, inLen, (const uint8_t **)m_audioFrameRaw->data, m_audioFrameRaw->nb_samples);
    *pts = getAudioFramPts(m_audioFrameRaw);
    av_frame_unref(m_audioFrameRaw);

    *outLen =  ret * para.frameSize;

    if (ret < 0)
        return false;
    else
        return true;
}


void MediaMainControl::cleanPktQueue()
{
    if (m_videoPktQueue != nullptr)
    {
        delete m_videoPktQueue;
        m_videoPktQueue = nullptr;
    }
    if (m_audioPktQueue != nullptr)
    {
        delete m_audioPktQueue;
        m_audioPktQueue = nullptr;
    }
}

void MediaMainControl::cleanFrameQueue()
{
    if (m_videoFrameQueue != nullptr)
    {
        delete m_videoFrameQueue;
        m_videoFrameQueue = nullptr;
    }
    if (m_audioFrameQueue != nullptr)
    {
        delete m_audioFrameQueue;
        m_audioFrameQueue = nullptr;
    }
}

void MediaMainControl::initPktQueue()
{
    cleanPktQueue();
    m_videoPktQueue = new PacketQueue();
    m_audioPktQueue = new PacketQueue();
    m_videoPktQueue->m_maxElements = maxVideoPktQueueSize;
    m_audioPktQueue->m_maxElements = maxVideoPktQueueSize;
}

void MediaMainControl::initFrameQueue()
{
    cleanFrameQueue();
    m_videoFrameQueue = new FrameQueue();
    m_audioFrameQueue = new FrameQueue();
    m_videoFrameQueue->m_maxElements = maxVideoFrameQueueSize;
    m_audioFrameQueue->m_maxElements = maxAudioFrameQueueSize;
}

void MediaMainControl::play()
{
    m_isQuit = false;
    initPktQueue();
    initFrameQueue();
    initSeparatePktThread(this);
    initDecodePktThread(this);

    //MediaDisplay *mediaDisplay = MediaDisplay::createDisplayInstance(this, DisplayType::USING_SDL);

    if (m_videoStreamIndex >= 0)
    {
        m_mediaDisplay->initVideoSetting(m_frameWidth, m_frameHeight, m_file.c_str());
        m_mediaDisplay->setVideoTimeBase(m_formatCtx->streams[m_videoStreamIndex]->time_base.num, m_formatCtx->streams[m_videoStreamIndex]->time_base.den);
        m_mediaDisplay->setFps(m_fps);
    }
    //if (m_audioStreamIndex >= 0)
    //{
    //    mediaDisplay->initAudioSetting(m_audioCodecCtx->sample_rate, m_audioCodecCtx->channels, m_audioCodecCtx->channel_layout);
    //    mediaDisplay->setAudioTimeBase(m_formatCtx->streams[m_audioStreamIndex]->time_base.num, m_formatCtx->streams[m_audioStreamIndex]->time_base.den);
    //}
    m_mediaDisplay->exec();
    MediaDisplay::destroyDisplayInstance(m_mediaDisplay);

}

void MediaMainControl::stop()
{

}