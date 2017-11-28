#include "mediaMainControl.h"

#include <iostream>
#include <windows.h>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "mediaDisplay.h"
namespace
{
    auto const errMsgBufferSize = 2048;
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
        if (m_totalTimeMS == -1)
            m_totalTimeMS = m_formatCtx->streams[m_videoStreamIndex]->duration * av_q2d(m_formatCtx->streams[m_videoStreamIndex]->time_base) * 1000;
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
    m_rect.x = 0;
    m_rect.y = 0;
    m_rect.w = m_videoCodecCtx->coded_width;
    m_rect.h = m_videoCodecCtx->coded_height;

    if (m_audioStreamIndex != -1)
        m_audioCodec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    if (m_audioCodec == nullptr)
        msgOutput("Not found audio decoder.");
    else
    {
        if(m_totalTimeMS == -1)
            m_totalTimeMS = m_formatCtx->streams[m_audioStreamIndex]->duration * av_q2d(m_formatCtx->streams[m_audioStreamIndex]->time_base);

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
    avcodec_close(m_videoCodecCtx);
    avcodec_close(m_audioCodecCtx);
    avcodec_free_context(&m_videoCodecCtx);
    avcodec_free_context(&m_audioCodecCtx);

    if (m_formatCtx)
        avformat_close_input(&m_formatCtx);
}

void MediaMainControl::initSeparatePktThread()
{
    cleanPktQueue();
    m_videoPktQueue = new PacketQueue();
    m_audioPktQueue = new PacketQueue();
    m_videoPktQueue->m_maxElements = 50;
    m_audioPktQueue->m_maxElements = 50;
    
    auto separatePktFun = [this](PacketQueue *videoPktQueue, PacketQueue *audioPktQueue, int audioInx,int videoInx, AVFormatContext *formatCtx, char* errMsgBuffer)
    {
        AVPacket *pAVPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
        while (true)
        {
            if (!formatCtx) return;
            int ret = av_read_frame(formatCtx, pAVPacket);
            if (ret != NULL)
            {
                if(ret == AVERROR_EOF) break;

                av_strerror(ret, errMsgBuffer, errMsgBufferSize);
                msgOutput(errMsgBuffer);
                continue;
            }
            //enqueue packet
            if (pAVPacket->stream_index == audioInx)
            {
                audioPktQueue->enQueue(pAVPacket);
                av_packet_unref(pAVPacket);
            }
            else if (pAVPacket->stream_index == videoInx)
            {
                videoPktQueue->enQueue(pAVPacket);
                av_packet_unref(pAVPacket);
            }
            else
                av_packet_unref(pAVPacket);
        }
        av_packet_free(&pAVPacket);
    };

    std::thread(separatePktFun, m_videoPktQueue, m_audioPktQueue, m_audioStreamIndex, m_videoStreamIndex, m_formatCtx, m_errMsgBuffer).detach();
}

void MediaMainControl::initDecodePktThread()
{
    cleanFrameQueue();
    m_videoFrameQueue = new FrameQueue();
    m_audioFrameQueue = new FrameQueue();
    m_videoFrameQueue->m_maxElements = 50;
    m_audioFrameQueue->m_maxElements = 100;

    auto decodePktFun = [&]()
    {
        AVFrame *frame = av_frame_alloc();
        AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        int ret = 0;
        while (true)
        {
            if (!m_formatCtx) return;
            //decode video pkt
            if (!m_videoPktQueue->m_queue.empty())
            {
                m_videoPktQueue->deQueue(packet);
                if (avcodec_send_packet(m_videoCodecCtx, packet) == NULL)
                {
                    if (avcodec_receive_frame(m_videoCodecCtx, frame) == NULL)
                    {
                        m_videoFrameQueue->enQueue(frame);
                        av_frame_unref(frame);
                    }
                }
                av_packet_unref(packet);
            }
            //decode audio pkt
            if (!m_audioPktQueue->m_queue.empty())
            {
                m_audioPktQueue->deQueue(packet);
                if (avcodec_send_packet(m_audioCodecCtx, packet) == NULL)
                {
                    while (avcodec_receive_frame(m_videoCodecCtx, frame) == NULL)
                    {
                        m_videoFrameQueue->enQueue(frame);
                        av_frame_unref(frame);
                    }
                }
                av_packet_unref(packet);
            }
        }
        av_packet_free(&packet);
        av_frame_free(&frame);
    };

    std::thread(decodePktFun).detach();
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

void MediaMainControl::play()
{
    initSeparatePktThread();
    initDecodePktThread();
    if (m_audioStreamIndex != -1)
        playAudio();
    if (m_videoStreamIndex != -1);
        playVideo();
}

void MediaMainControl::pause()
{

}

void MediaMainControl::stop()
{

}

void MediaMainControl::playAudio()
{

}

void MediaMainControl::playVideo()
{
    m_mediaDisplay = MediaDisplay::createSDLWindow(m_rect, "test", this);
    m_mediaDisplay->m_fps = m_fps;
    m_mediaDisplay->m_frameQueue = m_videoFrameQueue;
    m_mediaDisplay->exec();
}

void MediaMainControl::msgOutput(const char* msg)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::cout << "[Custom]: " << msg << std::endl;
}