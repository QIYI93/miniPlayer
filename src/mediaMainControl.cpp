#include "mediaMainControl.h"

#include <iostream>
#include <windows.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#include "mediaDisplay.h"
namespace
{
    auto const errMsgBufferSize = 2048;
    auto const maxAudioFrameSize = 192000;
    std::mutex s_mutex;
}

void msgOutput(const char* msg)
{
    std::unique_lock<std::mutex> lock(s_mutex);
    std::cout << "[Custom]: " << msg << std::endl;
}

MediaMainControl* MediaMainControl::getInstance()
{
    static MediaMainControl mediaMainControl;
    return &mediaMainControl;
}

MediaMainControl::MediaMainControl()
{
    av_register_all();
    m_errMsgBuffer = (char*)malloc(errMsgBufferSize * sizeof(char));
}

MediaMainControl::~MediaMainControl()
{
    free(m_errMsgBuffer);
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
        msgOutput(m_errMsgBuffer);
        return false;
    }
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0)
    {
        msgOutput("could not find codec parameters.");
        return false;
    }

    av_dump_format(m_formatCtx, 0, file, 0);
    if (m_formatCtx->duration != AV_NOPTS_VALUE)
        m_totalTimeMS = (m_formatCtx->duration / AV_TIME_BASE) * 1000;

    //Find video and audio stream
    m_videoStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, NULL);
    m_audioStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, NULL);

    //Open video and audio codec and create codec context
    if(m_videoStreamIndex != -1)
        m_videoCodec = avcodec_find_decoder(m_formatCtx->streams[m_videoStreamIndex]->codecpar->codec_id);
    if (m_videoCodec == nullptr)
        msgOutput("Not found video decoder.");
    else
    {
        if (m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate.den != NULL&&m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate.num != NULL)
            m_fps = av_q2d(m_formatCtx->streams[m_videoStreamIndex]->avg_frame_rate);

        m_videoCodecCtx = avcodec_alloc_context3(m_videoCodec);
        avcodec_parameters_to_context(m_videoCodecCtx, m_formatCtx->streams[m_videoStreamIndex]->codecpar);
        ret = avcodec_open2(m_videoCodecCtx, m_videoCodec, nullptr);
        if (ret != 0)
        {
            av_strerror(ret, m_errMsgBuffer, errMsgBufferSize);
            msgOutput(m_errMsgBuffer);
        }
    }
    m_frameWidth = m_videoCodecCtx->coded_width;
    m_frameHeight = m_videoCodecCtx->coded_height;

    if (m_audioStreamIndex != -1)
        m_audioCodec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    if (m_audioCodec == nullptr)
        msgOutput("Not found audio decoder.");
    else
    {
        m_audioCodecCtx = avcodec_alloc_context3(m_audioCodec);
        avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioStreamIndex]->codecpar);
        ret = avcodec_open2(m_audioCodecCtx, m_audioCodec, nullptr);
        if (ret != 0)
        {
            av_strerror(ret, m_errMsgBuffer, errMsgBufferSize);
            msgOutput(m_errMsgBuffer);
        }
    }
    return true;
}

void MediaMainControl::closeFile()
{
    cleanFrameQueue();
    cleanPktQueue();

    avcodec_close(m_videoCodecCtx);
    avcodec_close(m_audioCodecCtx);
    avcodec_free_context(&m_videoCodecCtx);
    avcodec_free_context(&m_audioCodecCtx);

    if (m_swsCtx)
        sws_freeContext(m_swsCtx);

    if (m_formatCtx)
        avformat_close_input(&m_formatCtx);
}

void MediaMainControl::initSeparatePktThread(void *mainCtrl)
{    
    auto separatePktFun = [](void *d)
    {
        MediaMainControl *mainCtl = static_cast<MediaMainControl*>(d);
        AVPacket *pAVPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
        int audioPktCount = 0;
        int videoPktCount = 0;
        while (true)
        {
            if (!mainCtl->m_formatCtx) return;
            int ret = av_read_frame(mainCtl->m_formatCtx, pAVPacket);
            if (ret != NULL)
            {
                if (ret == AVERROR_EOF)
                {
                    std::unique_lock<std::mutex> lock(mainCtl->m_mutex);
                    mainCtl->m_noPktToSperate = true;
                    break;
                }
                av_strerror(ret, mainCtl->m_errMsgBuffer, errMsgBufferSize);
                msgOutput(mainCtl->m_errMsgBuffer);
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

    std::thread(separatePktFun, mainCtrl).detach();
}

void MediaMainControl::initDecodePktThread(void *mainCtrl)
{
    auto decodeVideoPktFun = [&](void *d)
    {
        MediaMainControl *mainCtl = static_cast<MediaMainControl*>(d);
        AVFrame *frame = av_frame_alloc();
        AVPacket *videoPacket = av_packet_alloc();
        int videoFrameCount = 0;
        while (true)
        {
            if (!mainCtl->m_formatCtx) return;
            //decode video pkt
            if (!mainCtl->m_videoPktQueue->m_queue.empty())
            {
                mainCtl->m_videoPktQueue->deQueue(videoPacket);
                if (avcodec_send_packet(mainCtl->m_videoCodecCtx, videoPacket) == NULL)
                {
                    if (avcodec_receive_frame(mainCtl->m_videoCodecCtx, frame) == NULL)
                    {
                        printf("video frame count:%d\n", ++videoFrameCount);
                        mainCtl->m_videoFrameQueue->enQueue(frame);
                        av_frame_unref(frame);
                    }
                }
                av_packet_unref(videoPacket);
            }
            {
                std::unique_lock<std::mutex> lock(mainCtl->m_mutex);
                if (mainCtl->m_noPktToSperate && mainCtl->m_videoPktQueue->m_queue.empty())
                    mainCtl->m_videoFrameQueue->m_noMorePktToDecode = true;
                if(mainCtl->m_videoFrameQueue->m_noMorePktToDecode)
                    break;
            }
        }
        av_packet_free(&videoPacket);
        av_frame_free(&frame);
    };

    auto decodeAudioPktFun = [&](void *d)
    {
        MediaMainControl *mainCtl = static_cast<MediaMainControl*>(d);
        AVFrame *frame = av_frame_alloc();
        AVPacket *audioPacket = av_packet_alloc();
        int audioFrameCount = 0;
        while (true)
        {
            if (!mainCtl->m_formatCtx) return;
            //decode audio pkt
            if (!mainCtl->m_audioPktQueue->m_queue.empty())
            {
                mainCtl->m_audioPktQueue->deQueue(audioPacket);
                if (avcodec_send_packet(mainCtl->m_audioCodecCtx, audioPacket) == NULL)
                {
                    while (avcodec_receive_frame(mainCtl->m_audioCodecCtx, frame) == NULL)
                    {
                        printf("audio Frame Count:%d\n", ++audioFrameCount);
                        mainCtl->m_audioFrameQueue->enQueue(frame);
                        av_frame_unref(frame);
                    }
                }
                av_packet_unref(audioPacket);
            }
            {
                std::unique_lock<std::mutex> lock(mainCtl->m_mutex);
                if (mainCtl->m_noPktToSperate && mainCtl->m_audioPktQueue->m_queue.empty())
                    mainCtl->m_audioFrameQueue->m_noMorePktToDecode = true;
                if (mainCtl->m_audioFrameQueue->m_noMorePktToDecode)
                    break;
            }
        }
        av_packet_free(&audioPacket);
        av_frame_free(&frame);
    };

    std::thread(decodeVideoPktFun, mainCtrl).detach();
    std::thread(decodeAudioPktFun, mainCtrl).detach();
}

AVFrame* MediaMainControl::convertFrametoYUV420(AVFrame *src, const int width, const int height)
{
    static int h = 0;
    static int w = 0;
    static AVFrame *dst = nullptr;
    if (w != width && h != height)
    {
        w = width;
        h = height;
        if (dst)
            av_frame_free(&dst);
    }
    if (dst == nullptr)
    {
        dst = av_frame_alloc();
        unsigned char* outBuffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1));
        av_image_fill_arrays(dst->data, dst->linesize, outBuffer, AV_PIX_FMT_YUV420P, width, height, 1);
    }
    m_swsCtx = sws_getCachedContext(m_swsCtx,
        m_videoCodecCtx->width, m_videoCodecCtx->height,
        m_videoCodecCtx->pix_fmt,
        width, height,
        AV_PIX_FMT_YUV420P,
        SWS_BICUBIC,
        nullptr, nullptr, nullptr);
    if (!m_swsCtx)
    {
        msgOutput("sws_getCachedContext failed.");
        return dst;
    }
    int ret = sws_scale(m_swsCtx, (const uint8_t* const*)src->data, src->linesize, 0, m_videoCodecCtx->height, dst->data, dst->linesize);
    if (ret > 0)
    {
        return dst;
    }
    else
    {
        av_frame_free(&dst);
        return dst;
    }
}

int MediaMainControl::convertFrametoPCM(AVFrame* src, uint8_t *des, int inLen)
{
    if (!des || !inLen)
        return false;
    if(m_swrCtx == nullptr)
        m_swrCtx = swr_alloc();

    swr_alloc_set_opts(m_swrCtx, 
        m_audioParams.channelLayout, 
        m_audioParams.fmt, 
        m_audioParams.freq,
        m_audioCodecCtx->channel_layout,
        m_audioCodecCtx->sample_fmt, 
        m_audioCodecCtx->sample_rate,
        NULL, NULL);

    if (!m_swrCtx || swr_init(m_swrCtx) < 0)
    {
        av_log(NULL, AV_LOG_ERROR,
            "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
            m_audioCodecCtx->sample_rate, av_get_sample_fmt_name(m_audioCodecCtx->sample_fmt), m_audioCodecCtx->channels,
            m_audioParams.freq, av_get_sample_fmt_name(m_audioParams.fmt), m_audioParams.channels);
        swr_free(&m_swrCtx);
        return false;
    }

    int ret = swr_convert(m_swrCtx, &des, inLen, (const uint8_t **)src->data, src->nb_samples);
    return ret * m_audioParams.frameSize;
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
    m_videoPktQueue->m_maxElements = 50;
    m_audioPktQueue->m_maxElements = 50;
}

void MediaMainControl::initFrameQueue()
{
    cleanFrameQueue();
    m_videoFrameQueue = new FrameQueue();
    m_audioFrameQueue = new FrameQueue();
    m_videoFrameQueue->m_maxElements = 50;
    m_audioFrameQueue->m_maxElements = 50;
}

void MediaMainControl::play()
{
    initPktQueue();
    initFrameQueue();
    initSeparatePktThread(this);
    initDecodePktThread(this);

    MediaDisplay *mediaDisplay = MediaDisplay::createSDLInstance(this);
    if (m_videoStreamIndex != -1)
    {
        mediaDisplay->initVideoSetting(m_frameWidth, m_frameHeight, "window");
        mediaDisplay->setVideoFrameQueue(m_videoFrameQueue);
        mediaDisplay->setVideoTimeBase(m_formatCtx->streams[m_videoStreamIndex]->time_base);
        mediaDisplay->m_fps = m_fps;
    }
    if (m_audioStreamIndex != -1)
    {
        mediaDisplay->initAudioSetting(m_audioCodecCtx->sample_rate, m_audioCodecCtx->channels, m_audioCodecCtx->channel_layout, NULL, &m_audioParams);
        mediaDisplay->setAudioFrameQueue(m_audioFrameQueue);
        mediaDisplay->setAudioTimeBase(m_formatCtx->streams[m_audioStreamIndex]->time_base);
    }
    mediaDisplay->exec();
    MediaDisplay::destroySDLInstance(mediaDisplay);
}

void MediaMainControl::stop()
{

}