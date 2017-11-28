#include "mediaDisplay.h"

#include <map>
#include <algorithm>
#include <iostream>

using std::string;

namespace
{
    std::map<std::string, MediaDisplay*> windowList;
    std::mutex s_mutex;
}

MediaDisplay* MediaDisplay::createSDLWindow(VideoRect rect, const char *title, MediaMainControl *mainControl)
{
    if (windowList.find(title) != windowList.end())
    {
        msgOutput("Error,this window has exist.");
        return nullptr;
    }

    MediaDisplay *mediaDisplay = new MediaDisplay();
    windowList.insert(std::pair<std::string, MediaDisplay*>(title, mediaDisplay));

    SDL_Rect SDLRect;
    SDLRect.x = rect.x;
    SDLRect.y = rect.y;
    SDLRect.w = rect.w;
    SDLRect.h = rect.h;
    if (mediaDisplay->init(title, SDLRect, mainControl))
    {
        return mediaDisplay;
    }
    else
    {
        delete mediaDisplay;
        return nullptr;
    }
}

void MediaDisplay::destroyWindow(string title)
{
    auto ite = windowList.find(title);
    if (ite != windowList.end())
    {
        MediaDisplay *window = ite->second;
        delete window;
        windowList.erase(ite);
    }

}

MediaDisplay::MediaDisplay()
    :m_window(nullptr, SDL_DestroyWindow)
    , m_renderer(nullptr, SDL_DestroyRenderer)
    , m_texture(nullptr, SDL_DestroyTexture)
{

}

bool MediaDisplay::init(const char *title, SDL_Rect rect, MediaMainControl *mainControl)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
    {
        msgOutput("SDL Init Failed");
        return false;
    }

    m_windowRect = rect;

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

    m_texture.reset(SDL_CreateTexture(m_renderer.get(), SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, rect.w, rect.h));
    if (m_renderer == nullptr)
    {
        msgOutput("Failed to create texture");
        return false;
    }
    m_mainControl = mainControl;
    return true;
}

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
                event.type = SFM_REFRESH_EVENT;
                SDL_PushEvent(&event);
            }
            SDL_Delay(playState->delay); //25fps
        }
        SDL_Event event;
        event.type = SFM_BREAK_EVENT;
        SDL_PushEvent(&event);
    }
    return 0;
}

void MediaDisplay::exec()
{
    m_playState.delay = 1000 / m_fps;
    m_SDLEventThread = SDL_CreateThread(event_thread, NULL, &m_playState);

    AVFrame* frameRaw = nullptr;
    frameRaw = av_frame_alloc();

    while (true)
    {
        SDL_WaitEvent(&m_playState.SDLEvent);
        if (m_playState.SDLEvent.type == SFM_REFRESH_EVENT)
        {
            if(m_frameQueue->m_noMorePktToDecode && m_frameQueue->m_queue.empty())
                break;
            m_frameQueue->deQueue(frameRaw);
            AVFrame *frameYUV = m_mainControl->convertFrametoYUV420(frameRaw, m_windowRect.w, m_windowRect.h);
            av_frame_unref(frameRaw);
            if(frameYUV != nullptr && frameYUV->linesize[0] != 0)
                draw(frameYUV->data[0], frameYUV->linesize[0]);
        }
        else if (m_playState.SDLEvent.type == SDL_QUIT)
        {
            m_playState.exit = 1;
        }
        else if (m_playState.SDLEvent.type == SFM_BREAK_EVENT)
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