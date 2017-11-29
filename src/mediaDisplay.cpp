#include "mediaDisplay.h"

#include <iostream>

using std::string;

namespace
{
    std::mutex s_mutex;
    auto const REFRESH_VIDEO_EVENT = SDL_USEREVENT + 1;
    auto const REFRESH_AUDIO_EVENT = SDL_USEREVENT + 2;
    auto const BREAK_EVENT = SDL_USEREVENT + 3;
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

bool MediaDisplay::initAudioSetting(int freq, uint8_t channels, uint8_t silence, uint16_t samples)
{
    m_audioSpec.freq = freq;
    m_audioSpec.format = AUDIO_S16SYS;
    m_audioSpec.channels = channels;
    m_audioSpec.silence = silence;
    m_audioSpec.samples = samples;
    m_audioSpec.callback = fillAudioBuffer;
    m_audioSpec.userdata = &m_audioBuffer;

    if (SDL_OpenAudio(&m_audioSpec, NULL) < 0)
    {
        msgOutput("Failed to open audio");
        return false;
    }
    m_audioBuffer.PCMBufferSize = freq * channels;
    m_audioBuffer.PCMBuffer = (uint8_t *)av_malloc(m_audioBuffer.PCMBufferSize);
    return true;
}

static int event_thread(void* obaque)
{
    PlayState *playState = reinterpret_cast<PlayState*>(obaque);
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
            SDL_Delay(playState->delay); //25fps
        }
        SDL_Event event;
        event.type = BREAK_EVENT;
        SDL_PushEvent(&event);
    }
    return 0;
}

void MediaDisplay::exec()
{
    m_playState.delay = 1000 / m_fps;
    m_SDLEventThread = SDL_CreateThread(event_thread, NULL, &m_playState);

    AVFrame* videoFrameRaw = nullptr;
    videoFrameRaw = av_frame_alloc();
    AVFrame* audioFrameRaw = nullptr;
    audioFrameRaw = av_frame_alloc();

    while (true)
    {
        SDL_WaitEvent(&m_playState.SDLEvent);
        if (m_playState.SDLEvent.type == REFRESH_VIDEO_EVENT)
        {
            if(m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty())
                continue;
            m_videoFrameQueue->deQueue(videoFrameRaw);
            AVFrame *frameYUV = m_mainControl->convertFrametoYUV420(videoFrameRaw, m_windowRect.w, m_windowRect.h);
            av_frame_unref(videoFrameRaw);
            if(frameYUV != nullptr && frameYUV->linesize[0] != 0)
                draw(frameYUV->data[0], frameYUV->linesize[0]);
        }
        if (m_playState.SDLEvent.type == REFRESH_AUDIO_EVENT)
        {
            if (m_audioFrameQueue->m_noMorePktToDecode && m_audioFrameQueue->m_queue.empty())
                continue;
            m_audioFrameQueue->deQueue(audioFrameRaw);
            if (m_mainControl->convertFrametoPCM(audioFrameRaw, m_audioBuffer.PCMBuffer, m_audioBuffer.PCMBufferSize))
            {
                if (m_audioSpec.samples != audioFrameRaw->nb_samples) {
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
        else if (m_videoFrameQueue->m_noMorePktToDecode && m_videoFrameQueue->m_queue.empty()
            && m_audioFrameQueue->m_noMorePktToDecode && m_audioFrameQueue->m_queue.empty())
        {
            break;
        }
    }
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