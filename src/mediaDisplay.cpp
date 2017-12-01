#include "mediaDisplay.h"

#include <iostream>

using std::string;

namespace
{
    std::mutex s_mutex;
    auto const REFRESH_VIDEO_EVENT = SDL_USEREVENT + 1;
    auto const REFRESH_AUDIO_EVENT = SDL_USEREVENT + 2;
    auto const BREAK_EVENT = SDL_USEREVENT + 3;

    //copy from ffplay
    /* Minimum SDL audio buffer size, in samples. */    
    auto const SDLAudioMinBufferSize = 512;
    /* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
    auto const SDLAduioMaxCallBackPerSec = 30;
}

MediaDisplay* MediaDisplay::createSDLInstance(MediaMainControl *mainControl)
{
    MediaDisplay *mediaDisplay = new MediaDisplay(mainControl);

    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
    {
        msgOutput("SDL Init Failed");
        delete mediaDisplay;
        mediaDisplay = nullptr;
    }

    return mediaDisplay;
}

void MediaDisplay::destroySDLInstance(MediaDisplay *instance)
{
    if(instance != nullptr)
        delete instance;
}

MediaDisplay::MediaDisplay(MediaMainControl* mainControl)
    :m_window(nullptr, SDL_DestroyWindow)
    , m_mainControl(mainControl)
    , m_renderer(nullptr, SDL_DestroyRenderer)
    , m_texture(nullptr, SDL_DestroyTexture)
{
}

bool MediaDisplay::initVideoSetting(int width, int height, const char *title)
{
    m_windowRect.x = 0;
    m_windowRect.y = 0;
    m_windowRect.w = width;
    m_windowRect.h = height;

    m_window.reset(SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_windowRect.w, m_windowRect.h, SDL_WINDOW_SHOWN));
    if (m_window == nullptr)
    {
        msgOutput("Failed to create window");
        return false;
    }

    m_renderer.reset(SDL_CreateRenderer(m_window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (m_renderer == nullptr)
    {
        msgOutput("Failed to create renderer");
        return false;
    }

    m_texture.reset(SDL_CreateTexture(m_renderer.get(), SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_windowRect.w, m_windowRect.h));
    if (m_renderer == nullptr)
    {
        msgOutput("Failed to create texture");
        return false;
    }

    return true;
}

static void  fillAudioBuffer(void *udata, Uint8 *stream, int len)
{
    AudioBuffer *audioBuffer = static_cast<AudioBuffer*>(udata);
    SDL_memset(stream, 0, len);
    if (audioBuffer->PCMBufferSize == 0)
        return;
    len = (len > audioBuffer->restSize ? audioBuffer->restSize : len);

    SDL_MixAudio(stream, audioBuffer->pos, len, SDL_MIX_MAXVOLUME);
    audioBuffer->pos += len;
    audioBuffer->restSize -= len;

    if (audioBuffer->restSize == NULL)
    {
        SDL_Event event;
        event.type = REFRESH_AUDIO_EVENT;
        SDL_PushEvent(&event);
    }

}

bool MediaDisplay::initAudioSetting(int freq, uint8_t wantedChannels, uint64_t wantedChannelLayout, AudioParams *audioParams)
{
    //SDL_AudioSpec wantedSpec;
    //const char *env;
    //static const int nextChannels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
    //static const int nextSampleRates[] = { 0, 44100, 48000, 96000, 192000 };
    //int nextSampleRateIdx = FF_ARRAY_ELEMS(nextSampleRates) - 1;

    //env = SDL_getenv("SDL_AUDIO_CHANNELS");
    //if (env) {
    //    wantedChannels = atoi(env);
    //    wantedChannelLayout = av_get_default_channel_layout(wantedChannels);
    //}
    //if (!wantedChannelLayout || wantedChannels != av_get_channel_layout_nb_channels(wantedChannelLayout))
    //{
    //    wantedChannelLayout = av_get_default_channel_layout(wantedChannels);
    //    wantedChannelLayout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    //}
    //wantedChannels = av_get_channel_layout_nb_channels(wantedChannelLayout);
    //wantedSpec.channels = wantedChannels;
    //wantedSpec.freq = freq;
    //if (wantedSpec.freq <= 0 || wantedSpec.channels <= 0)
    //{
    //    av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
    //    return false;
    //}
    //while (nextSampleRateIdx && nextSampleRates[nextSampleRateIdx] >= wantedSpec.freq)
    //    nextSampleRateIdx--;
    //wantedSpec.format = AUDIO_S16SYS;
    //wantedSpec.silence = 0;
    //wantedSpec.samples = FFMAX(SDLAudioMinBufferSize, 2 << av_log2(wantedSpec.freq / SDLAduioMaxCallBackPerSec));
    //wantedSpec.callback = fillAudioBuffer;
    //wantedSpec.userdata = &m_audioBuffer;

    //while (SDL_OpenAudio(&wantedSpec, &m_audioSpec) < 0)
    //{
    //    av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
    //        wantedSpec.channels, wantedSpec.freq, SDL_GetError());
    //    wantedSpec.channels = nextChannels[FFMIN(7, wantedSpec.channels)];
    //    if (!wantedSpec.channels)
    //    {
    //        wantedSpec.freq = nextSampleRates[nextSampleRateIdx--];
    //        wantedSpec.channels = wantedChannels;
    //        if (!wantedSpec.freq)
    //        {
    //            av_log(NULL, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
    //            return false;
    //        }
    //    }
    //    wantedChannelLayout = av_get_default_channel_layout(wantedSpec.channels);
    //}
    //if (m_audioSpec.format != AUDIO_S16SYS)
    //{
    //    av_log(NULL, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", m_audioSpec.format);
    //    return false;
    //}
    //if (m_audioSpec.channels != wantedSpec.channels)
    //{
    //    wantedChannelLayout = av_get_default_channel_layout(m_audioSpec.channels);
    //    if (!wantedChannelLayout)
    //    {
    //        av_log(NULL, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", m_audioSpec.channels);
    //        return -1;
    //    }
    //}
    //audioParams->fmt = AV_SAMPLE_FMT_S16;
    //audioParams->freq = m_audioSpec.freq;
    //audioParams->channelLayout = wantedChannelLayout;
    //audioParams->channels = m_audioSpec.channels;
    //audioParams->frameSize = av_samples_get_buffer_size(NULL, audioParams->channels, 1, audioParams->fmt, 1);
    //audioParams->bytesPerSec = av_samples_get_buffer_size(NULL, audioParams->channels, audioParams->freq, audioParams->fmt, 1);
    //if (audioParams->bytesPerSec <= 0 || audioParams->frameSize <= 0)
    //{
    //    av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
    //    return false;
    //}


    //return true;

    m_audioSpec.freq = freq;
    m_audioSpec.format = AUDIO_S16SYS;
    m_audioSpec.channels = wantedChannels;
    m_audioSpec.silence = 0;
    m_audioSpec.samples = 1024;
    m_audioSpec.callback = fillAudioBuffer;
    m_audioSpec.userdata = &m_audioBuffer;

    if (SDL_OpenAudio(&m_audioSpec, NULL) < 0)
    {
        msgOutput("Failed to open audio");
        return false;
    }
    m_audioBuffer.PCMBufferSize = freq * wantedChannels;
    m_audioBuffer.PCMBuffer = (uint8_t *)av_malloc(m_audioBuffer.PCMBufferSize);

    m_playState.audioFrameDuration = m_audioSpec.samples / freq;
    return true;
}

static int event_thread(void* obaque)
{
    PlayState *playState = reinterpret_cast<PlayState*>(obaque);
    //init audio event
    SDL_Event event;
    event.type = REFRESH_AUDIO_EVENT;
    SDL_PushEvent(&event);
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
    }
    return 0;
}

void MediaDisplay::exec()
{
    //if(m_audioFrameQueue == nullptr)
        m_playState.delay = 1000 / m_fps;

    m_SDLEventThread = SDL_CreateThread(event_thread, NULL, &m_playState);

    AVFrame* videoFrameRaw = nullptr;
    videoFrameRaw = av_frame_alloc();
    AVFrame* audioFrameRaw = nullptr;
    audioFrameRaw = av_frame_alloc();
    while (true)
    {
        SDL_WaitEvent(&m_playState.SDLEvent);
        if (m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty()
            && m_audioFrameQueue->m_noMorePktToDecode && m_audioFrameQueue->m_queue.empty())
        {
            break;
        }
        else if (m_playState.SDLEvent.type == REFRESH_VIDEO_EVENT)
        {
            if(m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty())
                continue;
            m_videoFrameQueue->deQueue(videoFrameRaw);

            int curPts = av_frame_get_best_effort_timestamp(videoFrameRaw);
            if (curPts != AV_NOPTS_VALUE)
            {
                m_playState.currentVideoTime = curPts * av_q2d(m_videoTimeBsse) * 1000;
                m_playState.videoPreFrameDelay = (curPts - m_playState.videoPrePts) * av_q2d(m_videoTimeBsse) * 1000;
                m_playState.videoPrePts = curPts;
            }
            getDelay();

            AVFrame *frameYUV = m_mainControl->convertFrametoYUV420(videoFrameRaw, m_windowRect.w, m_windowRect.h);
            av_frame_unref(videoFrameRaw);
            if(frameYUV != nullptr && frameYUV->linesize[0] != 0)
                draw(frameYUV->data[0], frameYUV->linesize[0]);
        }
        else if (m_playState.SDLEvent.type == REFRESH_AUDIO_EVENT)
        {
            if (m_audioFrameQueue->m_noMorePktToDecode && m_audioFrameQueue->m_queue.empty())
                continue;
            m_audioFrameQueue->deQueue(audioFrameRaw);
            if (m_mainControl->convertFrametoPCM(audioFrameRaw, m_audioBuffer.PCMBuffer, m_audioBuffer.PCMBufferSize))
            {
                if (audioFrameRaw->pkt_pts != AV_NOPTS_VALUE)
                    m_playState.currentAudioTime = audioFrameRaw->pts * av_q2d(m_audioTimeBase) * 1000;
                else
                    m_playState.currentAudioTime += m_playState.audioFrameDuration;
                if (m_audioSpec.samples != audioFrameRaw->nb_samples)
                {
                    m_playState.audioFrameDuration = (double)audioFrameRaw->nb_samples / (double)m_audioSpec.freq * 1000;
                    SDL_CloseAudio();
                    m_audioSpec.samples = audioFrameRaw->nb_samples;
                    m_audioBuffer.PCMBufferSize = av_samples_get_buffer_size(NULL, m_audioSpec.channels, m_audioSpec.samples, AV_SAMPLE_FMT_S16, 1);
                    SDL_OpenAudio(&m_audioSpec, NULL);
                }
                m_audioBuffer.restSize = m_audioBuffer.PCMBufferSize;
                m_audioBuffer.pos = m_audioBuffer.PCMBuffer;
                SDL_PauseAudio(0);
            }
            av_frame_unref(audioFrameRaw);
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
}

void MediaDisplay::getDelay()
{
    double 		retDelay = 0.0;
    double 		compare = 0.0;
    double  	threshold = 0.0;

    if (m_playState.currentVideoTime == 0 && m_playState.currentAudioTime == 0)
    {
        m_playState.delay = m_fps;
        return;
    }

    compare = m_playState.currentVideoTime - m_playState.currentAudioTime;
    threshold = m_playState.videoPreFrameDelay;
    if (compare <= -threshold)
        retDelay = threshold / 2;
    else if (compare >= threshold)
        retDelay = threshold * 2;
    else
        retDelay = threshold;

    m_playState.delay = retDelay;

    av_log(nullptr, AV_LOG_INFO, "currentVideoTime:%lf,currentAudioTime:%lf,compare:%lf,preFrameDelay:%lf\n",
        m_playState.currentVideoTime, m_playState.currentAudioTime, compare, retDelay);
}

void MediaDisplay::draw(const uint8_t *data, const int lineSize)
{
    SDL_UpdateTexture(m_texture.get(), nullptr, data, lineSize);
    SDL_RenderClear(m_renderer.get());
    SDL_RenderCopy(m_renderer.get(), m_texture.get(), nullptr, nullptr);
    SDL_RenderPresent(m_renderer.get());
}

MediaDisplay::~MediaDisplay()
{
    SDL_Quit();
}

void MediaDisplay::msgOutput(const char* msg)
{
    std::unique_lock<std::mutex> lock(s_mutex);
    std::cout << "[SDL]: " << msg << std::endl;
}