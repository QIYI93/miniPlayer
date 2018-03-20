#ifndef MEDIADISPLAY_SDL_H
#define MEDIADISPLAY_SDL_H

#include <memory>
#include <mutex>

extern "C"
{
#include "SDL_syswm.h"
#include "SDL.h"
}
#include "mediaDisplay.h"

typedef struct videoBuffer
{
    uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
    int  lineSize[AV_NUM_DATA_POINTERS] = { 0 };
    uint32_t size;
}videoBuffer;

typedef struct AudioBuffer
{
    uint8_t *PCMBuffer = nullptr;
    uint8_t *pos = 0;
    int PCMBufferSize = 0;
    int restSize = 0;
    int bytesPerSec = 0;
}AudioBuffer;

class MediaDisplay_SDL : public MediaDisplay
{
public:
    MediaDisplay_SDL(MediaMainControl *mainCtrl);

    virtual bool init() override;
    virtual bool initVideoSetting(int width, int height, const char *title) override;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout) override;

    virtual void exec() override;
    virtual HWND getWinHandle() override;

private:
    MediaDisplay_SDL(MediaDisplay_SDL&) = delete;
    ~MediaDisplay_SDL();

    static void fillAudioBuffer(void *udata, Uint8 *stream, int len);
    void draw(const uint8_t *data, const int lineSize);
    void getDelay();

private:
    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> m_window;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> m_renderer;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> m_texture;

    SDL_Rect m_windowRect;
    SDL_Thread* m_SDLEventThread = nullptr;
    SDL_Event m_SDLEvent;

    videoBuffer m_videoBuffer;
    AudioBuffer m_audioBuffer;

    SDL_AudioSpec m_audioSpec;

    bool m_quit = false;

};

#endif