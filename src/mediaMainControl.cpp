#include "mediaMainControl.h"
#include <windows.h>

extern "C"
{
    #include <libavformat/avformat.h>
}

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

}

bool MediaMainControl::openFile(const char *file)
{
    closeFile();
    m_formatCtx = avformat_alloc_context();
    int ret = avformat_open_input(&m_formatCtx, file, nullptr, nullptr);
    if (ret != 0)
    {
        av_strerror(ret, m_errMsgBuffer, errMsgBufferSize);
        OutputDebugStringA(m_errMsgBuffer);
        return false;
    }
    av_dump_format(m_formatCtx, 0, file, 0);
    if (m_formatCtx->duration != AV_NOPTS_VALUE)
        m_totalTimeMS = (m_formatCtx->duration / AV_TIME_BASE) * 1000;

    //Find video and audio stream
    int m_videoStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, NULL);
    int m_audioStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, NULL);

    //Open video and audio codec and create codec context
    if(m_videoStreamIndex != -1)
        m_videoCodec = avcodec_find_decoder(m_formatCtx->streams[m_videoStreamIndex]->codecpar->codec_id);
    if (m_videoCodec == nullptr)
        OutputDebugStringA("Not found video decoder.");
    else
    {
        if (m_totalTimeMS == -1)
            m_totalTimeMS = m_formatCtx->streams[m_videoStreamIndex]->duration * av_q2d(m_formatCtx->streams[m_videoStreamIndex]->time_base) * 1000;

        m_videoCodecCtx = avcodec_alloc_context3(m_videoCodec);
        avcodec_parameters_to_context(m_videoCodecCtx, m_formatCtx->streams[m_videoStreamIndex]->codecpar);
        ret = avcodec_open2(m_videoCodecCtx, m_videoCodec, nullptr);
        if (ret != 0)
        {
            av_strerror(ret, m_errMsgBuffer, sizeof(m_errMsgBuffer));
            OutputDebugStringA(m_errMsgBuffer);
        }
    }

    if (m_audioStreamIndex != -1)
        m_audioCodec = avcodec_find_decoder(m_formatCtx->streams[m_audioStreamIndex]->codecpar->codec_id);
    if (m_audioCodec == nullptr)
        OutputDebugStringA("Not found audio decoder.");
    else
    {
        if(m_totalTimeMS == -1)
            m_totalTimeMS = m_formatCtx->streams[m_audioStreamIndex]->duration * av_q2d(m_formatCtx->streams[m_audioStreamIndex]->time_base);

        m_audioCodecCtx = avcodec_alloc_context3(m_audioCodec);
        avcodec_parameters_to_context(m_audioCodecCtx, m_formatCtx->streams[m_audioStreamIndex]->codecpar);
        ret = avcodec_open2(m_audioCodecCtx, m_audioCodec, nullptr);
        if (ret != 0)
        {
            av_strerror(ret, m_errMsgBuffer, sizeof(m_errMsgBuffer));
            OutputDebugStringA(m_errMsgBuffer);
        }
    }
    return true;
}

void MediaMainControl::closeFile()
{
    avcodec_close(m_videoCodecCtx);
    avcodec_close(m_audioCodecCtx);
    m_videoCodecCtx = nullptr;
    m_audioCodecCtx = nullptr;

    if (m_formatCtx)
        avformat_close_input(&m_formatCtx);
}

void MediaMainControl::initSeparatePkt()
{

}
