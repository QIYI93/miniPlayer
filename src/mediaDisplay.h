#ifndef MEDIADISPLAY_H
#define MEDIADISPLAY_H

#include <memory>
#include <mutex>
#include "SDL.h"
#include "pktAndFramequeue.h"
#include "mediaMainControl.h"

#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

typedef struct PlayState
{
    int threadPause = 0;
    int threadExit = 0;
    int delay = 0;
    SDL_Event SDLEvent;
}PlayState;

class MediaMainControl;
class MediaDisplay
{
public:
    static MediaDisplay *createSDLWindow(VideoRect rect, const char *title = "Window", MediaMainControl *mainControl = nullptr);
    void draw(const uint8_t *data, const int lineSize);
    void exec();
    void quit();

    int m_fps = 0;
    FrameQueue *m_frameQueue = nullptr;

private:
    MediaDisplay() = default;
    bool init(const char *title, SDL_Rect rect, MediaMainControl *mainControl);

    ~MediaDisplay();

    static void msgOutput(const char*);

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture *m_texture = nullptr;
    SDL_Rect m_windowRect;
    //SDL_Thread* m_SDLEventThread = nullptr;
    SDL_Thread* m_SDLEventThread = nullptr;
    PlayState m_playState;

    MediaMainControl *m_mainControl = nullptr;
};




#endif
/*

using std::string;
class SDLWindow
{
public:
    SDLWindow();

    void Init(string title = "Window");
    void Quit();
    void Draw(SDL_Texture *tex, SDL_Rect &dstRect, SDL_Rect *clip = NULL, float angle = 0.0, int xPivot = 0, int yPivot = 0, SDL_RendererFlip flip = SDL_FLIP_NONE);
    SDL_Texture* loadImage(const string &file);
    SDL_Texture* RenderText(const string &message, const string &fontFile, SDL_Color color, int fontSize);
    void Clear();
    void Present();
    SDL_Rect Box();

    ~SDLWindow();

private:
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> mWindow;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> mRenderer;
    SDL_Rect mBox;
    */