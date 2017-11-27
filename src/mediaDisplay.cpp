#include "mediaDisplay.h"

#include <map>
#include <vector>
#include <algorithm>
#include <iostream>

using std::string;

namespace
{
    std::vector<std::string> windowList;
}

MediaDisplay* MediaDisplay::createSDLWindow(const char *title /* = "Window" */, SDL_Rect rect /* = SDL_Rect() */)
{
    auto ite = std::find_if(windowList.cbegin(), windowList.cend(), [title](const string item)
    { return item == string(title); });
    if (ite != windowList.cend())
    {
        msgOutput("Error,this window has exist.");
        return nullptr;
    }

    windowList.push_back(title);
    MediaDisplay *mediaDisplay = new MediaDisplay();
    if (mediaDisplay->init(title, rect))
    {
        return mediaDisplay;
    }
    else
    {
        delete mediaDisplay;
        return nullptr;
    }
}

static int event_thread(void* obaque)
{
    EventStruct *event = reinterpret_cast<EventStruct*>(obaque);
    while (true)
    {
        while (!event->threadExit)
        {
            if (!event->threadPause)
            {
                SDL_Event event;
                event.type = SFM_REFRESH_EVENT;
                SDL_PushEvent(&event);
            }
            SDL_Delay(40); //25fps
        }
        SDL_Event event;
        event.type = SFM_BREAK_EVENT;
        SDL_PushEvent(&event);
    }
    return 0;
}

bool MediaDisplay::init(const char *title, SDL_Rect rect)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
    {
        msgOutput("SDL Init Failed");
        return false;
    }

    m_windowRect = rect;

    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_windowRect.w, m_windowRect.h, SDL_WINDOW_SHOWN);
    if (m_window == nullptr)
    {
        msgOutput("Failed to create window");
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (m_renderer == nullptr)
    {
        msgOutput("Failed to create renderer");
        return false;
    }

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, rect.w, rect.h);
    if (m_renderer == nullptr)
    {
        msgOutput("Failed to create texture");
        return false;
    }
    return true;
}

void MediaDisplay::show()
{
    //m_SDLEventThread = SDL_CreateThread(event_thread, NULL, &m_event);
    //while (true)
    //{
        SDL_PollEvent(&m_event.SDLEvent);
    //}
}

void MediaDisplay::draw(const uint8_t *data, const uint32_t lineSize)
{
    SDL_UpdateTexture(m_texture, nullptr, data, lineSize);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void MediaDisplay::quit()
{

}

MediaDisplay::~MediaDisplay()
{
}

void MediaDisplay::msgOutput(const char* msg)
{
    std::cout << "[SDL]: " << msg << std::endl;
}