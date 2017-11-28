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
    bool pause = false;
    bool exit = false;
    int delay = 0;
    SDL_Event SDLEvent;
}PlayState;

class MediaMainControl;
class MediaDisplay
{
public:
    static MediaDisplay *createSDLWindow(VideoRect rect, const char *title = "Window", MediaMainControl *mainControl = nullptr);
    static void destroyWindow(std::string title);
    void draw(const uint8_t *data, const int lineSize);
    void exec();

    int m_fps = 0;
    FrameQueue *m_frameQueue = nullptr;

private:
    MediaDisplay();
    bool init(const char *title, SDL_Rect rect, MediaMainControl *mainControl);

    ~MediaDisplay();

    static void msgOutput(const char*);

private:
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> m_window;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> m_renderer;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> m_texture;

    SDL_Rect m_windowRect;
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