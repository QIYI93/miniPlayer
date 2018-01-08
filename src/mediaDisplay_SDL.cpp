#include "mediaDisplay_SDL.h"

#include <iostream>

extern "C"
{
//#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

using std::string;

namespace
{
    std::mutex s_mutex;
    auto const REFRESH_VIDEO_EVENT = SDL_USEREVENT + 1;
    auto const REFRESH_AUDIO_EVENT = SDL_USEREVENT + 2;
    auto const BREAK_EVENT = SDL_USEREVENT + 3;

    //copy from ffplay
    auto const SDLAudioMinBufferSize = 512;
    auto const SDLAduioMaxCallBackPerSec = 30;
}

//MediaDisplay* MediaDisplay::createSDLInstance(MediaMainControl *mainControl)
//{

//}
//
//void MediaDisplay::destroySDLInstance(MediaDisplay *instance)
//{
//    if (instance != nullptr)
//        delete instance;
//}
//
MediaDisplay_SDL::MediaDisplay_SDL(MediaMainControl *mainCtrl)
    :MediaDisplay(mainCtrl)
    , m_window(nullptr, SDL_DestroyWindow)
    , m_renderer(nullptr, SDL_DestroyRenderer)
    , m_texture(nullptr, SDL_DestroyTexture)
{
    m_displayType = DisplayType::USING_SDL;
}

MediaDisplay_SDL::~MediaDisplay_SDL()
{
    SDL_Quit();
    free(m_videoBuffer.data);
}

bool MediaDisplay_SDL::init()
{
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) == -1)
    {
        msgOutput(MsgType::MSG_ERROR, "SDL Init Failed.");
        return false;
    }
    return true;
}


bool MediaDisplay_SDL::initVideoSetting(int width, int height, const char *title)
{
    m_windowRect.x = 0;
    m_windowRect.y = 0;
    m_windowRect.w = width;
    m_windowRect.h = height;

    m_window.reset(SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_windowRect.w, m_windowRect.h, SDL_WINDOW_SHOWN));
    if (m_window == nullptr)
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create window.");
        return false;
    }

    m_renderer.reset(SDL_CreateRenderer(m_window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (m_renderer == nullptr)
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create renderer.");
        return false;
    }

    m_texture.reset(SDL_CreateTexture(m_renderer.get(), SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_windowRect.w, m_windowRect.h));
    if (m_renderer == nullptr)
    {
        msgOutput(MsgType::MSG_ERROR, "Failed to create texture.");
        return false;
    }
    
    m_playState.videoDisplay = true;
    return true;
}
//
//void MediaDisplay::fillAudioBuffer(void *udata, Uint8 *stream, int len)
//{
//    static AVFrame* audioFrameRaw = av_frame_alloc();
//
//    MediaDisplay *mediaDisplay = static_cast<MediaDisplay*>(udata);
//    AudioBuffer *audioBuffer = &mediaDisplay->m_audioBuffer;
//    SDL_memset(stream, 0, len);
//    int len1;
//    int index = 0;
//    if (audioBuffer->PCMBufferSize == 0)
//        return;
//
//    while (len > 0)
//    {
//        if (audioBuffer->restSize <= 0)
//        {
//            if (mediaDisplay->m_audioFrameQueue->m_queue.empty())
//                return;
//            mediaDisplay->m_audioFrameQueue->deQueue(audioFrameRaw);
//            int outLen = mediaDisplay->m_mainControl->convertFrametoPCM(audioFrameRaw, audioBuffer->PCMBuffer, audioBuffer->PCMBufferSize);
//            av_frame_unref(audioFrameRaw);
//            audioBuffer->pos = audioBuffer->PCMBuffer;
//            audioBuffer->restSize = outLen;
//        }
//        len1 = (len > audioBuffer->restSize ? audioBuffer->restSize : len);
//        SDL_MixAudio(stream + index, audioBuffer->pos, len1, SDL_MIX_MAXVOLUME);
//        index += len1;
//        audioBuffer->pos += len1;
//        audioBuffer->restSize -= len1;
//        len -= len1;
//    }
//}
//
//bool MediaDisplay::initAudioSetting(int freq, uint8_t wantedChannels, uint64_t wantedChannelLayout, uint64_t sample, AudioParams *audioParams)
//{
//    SDL_AudioSpec wantedSpec;
//    const char *env;
//    static const int nextChannels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
//    static const int nextSampleRates[] = { 0, 44100, 48000, 96000, 192000 };
//    int nextSampleRateIdx = FF_ARRAY_ELEMS(nextSampleRates) - 1;
//
//    env = SDL_getenv("SDL_AUDIO_CHANNELS");
//    if (env) {
//        wantedChannels = atoi(env);
//        wantedChannelLayout = av_get_default_channel_layout(wantedChannels);
//    }
//    if (!wantedChannelLayout || wantedChannels != av_get_channel_layout_nb_channels(wantedChannelLayout))
//    {
//        wantedChannelLayout = av_get_default_channel_layout(wantedChannels);
//        wantedChannelLayout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
//    }
//    wantedChannels = av_get_channel_layout_nb_channels(wantedChannelLayout);
//    wantedSpec.channels = wantedChannels;
//    wantedSpec.freq = freq;
//    if (wantedSpec.freq <= 0 || wantedSpec.channels <= 0)
//    {
//        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
//        return false;
//    }
//    while (nextSampleRateIdx && nextSampleRates[nextSampleRateIdx] >= wantedSpec.freq)
//        nextSampleRateIdx--;
//    wantedSpec.format = AUDIO_S16SYS;
//    wantedSpec.silence = 0;
//    wantedSpec.samples = FFMAX(SDLAudioMinBufferSize, 2 << av_log2(wantedSpec.freq / SDLAduioMaxCallBackPerSec));
//    wantedSpec.callback = fillAudioBuffer;
//    wantedSpec.userdata = this;
//
//    while (SDL_OpenAudio(&wantedSpec, &m_audioSpec) < 0)
//    {
//        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
//            wantedSpec.channels, wantedSpec.freq, SDL_GetError());
//        wantedSpec.channels = nextChannels[FFMIN(7, wantedSpec.channels)];
//        if (!wantedSpec.channels)
//        {
//            wantedSpec.freq = nextSampleRates[nextSampleRateIdx--];
//            wantedSpec.channels = wantedChannels;
//            if (!wantedSpec.freq)
//            {
//                av_log(NULL, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
//                return false;
//            }
//        }
//        wantedChannelLayout = av_get_default_channel_layout(wantedSpec.channels);
//    }
//    if (m_audioSpec.format != AUDIO_S16SYS)
//    {
//        av_log(NULL, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", m_audioSpec.format);
//        return false;
//    }
//    if (m_audioSpec.channels != wantedSpec.channels)
//    {
//        wantedChannelLayout = av_get_default_channel_layout(m_audioSpec.channels);
//        if (!wantedChannelLayout)
//        {
//            av_log(NULL, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", m_audioSpec.channels);
//            return -1;
//        }
//    }
//    audioParams->fmt = AV_SAMPLE_FMT_S16;
//    audioParams->freq = m_audioSpec.freq;
//    audioParams->channelLayout = wantedChannelLayout;
//    audioParams->channels = m_audioSpec.channels;
//    audioParams->frameSize = av_samples_get_buffer_size(NULL, audioParams->channels, 1, audioParams->fmt, 1);
//    audioParams->bytesPerSec = av_samples_get_buffer_size(NULL, audioParams->channels, audioParams->freq, audioParams->fmt, 1);
//    if (audioParams->bytesPerSec <= 0 || audioParams->frameSize <= 0)
//    {
//        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
//        return false;
//    }
//
//    m_audioBuffer.PCMBufferSize = av_samples_get_buffer_size(NULL, m_audioSpec.channels, m_audioSpec.samples, AV_SAMPLE_FMT_S16, 1);
//    m_audioBuffer.PCMBuffer = (uint8_t *)av_malloc(m_audioBuffer.PCMBufferSize);
//
//    m_playState.audioDisplay = true;
//    return true;
//
//}

static int event_thread(void* obaque)
{
    PlayState *playState = reinterpret_cast<PlayState*>(obaque);
    while (true)
    {
        while (!playState->exit)
        {
            if (!playState->pause)
            {
                SDL_Event event;
                event.type = REFRESH_VIDEO_EVENT;
                SDL_PushEvent(&event);
            }
            SDL_Delay(playState->delay);
        }
        SDL_Event event;
        event.type = BREAK_EVENT;
        SDL_PushEvent(&event);
        break;
    }
    return 0;
}

void MediaDisplay_SDL::exec()
{
    AVFrame* videoFrameRaw = nullptr;
    if (m_playState.videoDisplay)
    {
        m_SDLEventThread = SDL_CreateThread(event_thread, NULL, &m_playState);
        videoFrameRaw = av_frame_alloc();
        if (m_fps > 0)
            m_playState.delay = 1000 / m_fps;
    }

    //if (m_playState.audioDisplay)
    //    SDL_PauseAudio(0);

    while (!m_quit)
    {
        SDL_WaitEvent(&m_playState.SDLEvent);
        if (m_mainCtrl->isAudioFrameEmpty() && m_mainCtrl->isVideoFrameEmpty())
        {
            m_playState.exit = 1;
        }
        if (m_playState.SDLEvent.type == REFRESH_VIDEO_EVENT)
        {
            if (m_mainCtrl->isVideoFrameEmpty())
                continue;

            //initialize buffer and change space when window rect is changing
            int w, h;
            int64_t pts = 0;
            SDL_GetWindowSize(m_window.get(), &w, &h);
            if (m_videoBuffer.data[0] == nullptr || w != m_windowRect.w || h != m_windowRect.h)
            {
                if (m_videoBuffer.data[0] != nullptr)
                    free(m_videoBuffer.data[0]);
                m_videoBuffer.size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, w, h, 1);
                unsigned char* outBuffer = (unsigned char*)av_malloc(m_videoBuffer.size);
                av_image_fill_arrays(m_videoBuffer.data, m_videoBuffer.lineSize, outBuffer, AV_PIX_FMT_YUV420P, m_windowRect.w, m_windowRect.h, 1);
                m_windowRect.w = w;
                m_windowRect.h = h;
            }
            if (m_mainCtrl->getGraphicData(GraphicDataType::YUV420, m_windowRect.w, m_windowRect.h, m_videoBuffer.data, m_videoBuffer.size, m_videoBuffer.lineSize, &pts))
            {
                //getDelay();
                draw(m_videoBuffer.data[0], m_videoBuffer.lineSize[0]);
            }
            else
            {
                msgOutput(MsgType::MSG_WARNING, "convert data to YUV420 format failed.");
                continue;
            }

            //m_videoFrameQueue->deQueue(videoFrameRaw);

            int curPts = av_frame_get_best_effort_timestamp(videoFrameRaw);
            if (curPts != AV_NOPTS_VALUE)
            {
                m_playState.currentVideoTime = curPts * MediaDisplay::r2d(m_videoTimeBsse) * 1000;
                m_playState.videoPreFrameDelay = (curPts - m_playState.videoPrePts) * MediaDisplay::r2d(m_videoTimeBsse) * 1000;
                m_playState.videoPrePts = curPts;
            }


            //AVFrame *frameYUV = m_mainControl->convertFrametoYUV420(videoFrameRaw, m_windowRect.w, m_windowRect.h);
            //av_frame_unref(videoFrameRaw);
            //if (frameYUV != nullptr && frameYUV->linesize[0] != 0)
        }
        else if (m_playState.SDLEvent.type == SDL_QUIT)
        {
            m_playState.exit = 1;
        }
        else if (m_playState.SDLEvent.type == BREAK_EVENT)
        {
            break;
        }
    }
    av_frame_free(&videoFrameRaw);
}

//void MediaDisplay::getDelay()
//{
//    double 		retDelay = 0.0;
//    double 		compare = 0.0;
//    double  	threshold = 0.0;
//
//    if (m_playState.currentVideoTime == 0 && m_playState.currentAudioTime == 0)
//    {
//        m_playState.delay = m_fps;
//        return;
//    }
//
//    compare = m_playState.currentVideoTime - m_playState.currentAudioTime;
//    threshold = m_playState.videoPreFrameDelay;
//    if (compare <= -threshold)
//        retDelay = threshold / 2;
//    else if (compare >= threshold)
//        retDelay = threshold * 2;
//    else
//        retDelay = threshold;
//
//    m_playState.delay = retDelay;
//
//    av_log(nullptr, AV_LOG_INFO, "currentVideoTime:%lf,currentAudioTime:%lf,compare:%lf,preFrameDelay:%lf\n",
//        m_playState.currentVideoTime, m_playState.currentAudioTime, compare, retDelay);
//}

void MediaDisplay_SDL::draw(const uint8_t *data, const int lineSize)
{
    SDL_UpdateTexture(m_texture.get(), nullptr, data, lineSize);
    SDL_RenderClear(m_renderer.get());
    SDL_RenderCopy(m_renderer.get(), m_texture.get(), nullptr, nullptr);
    SDL_RenderPresent(m_renderer.get());
}