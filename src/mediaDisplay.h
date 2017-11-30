#ifndef MEDIADISPLAY_H
#define MEDIADISPLAY_H

#include <memory>
#include <mutex>

extern "C"
{
#include "SDL.h"
}
#include "pktAndFramequeue.h"
#include "mediaMainControl.h"

typedef struct PlayState
{
    bool pause = false;
    bool exit = false;
    int delay = 0;
    double audioFrameDuration = 0.0;
    double currentAudioTime = 0.0;
    double currentVideoTime = 0.0;
    int videoPrePts = 0;
    double videoPreFrameDelay = 0.0;
    SDL_Event SDLEvent;
}PlayState;

typedef struct AudioBuffer
{
    uint8_t *PCMBuffer = nullptr;
    int PCMBufferSize = 0;
    uint8_t *pos = 0;
    int restSize = 0;
}AudioBuffer;

class MediaMainControl;
class MediaDisplay
{
public:
    static MediaDisplay *createSDLInstance(MediaMainControl *mainControl = nullptr);
    static void destroySDLInstance(MediaDisplay*);

    bool initVideoSetting(int width, int height, const char *title);
    bool initAudioSetting(int freq, uint8_t channels, uint8_t silence, uint16_t samples);

    void exec();

    void setVideoFrameQueue(FrameQueue* queue) { m_videoFrameQueue = queue; }
    void setAudioFrameQueue(FrameQueue* queue) { m_audioFrameQueue = queue; }
    void setVideoTimeBase(AVRational videoTimeBsse) { m_videoTimeBsse = videoTimeBsse; }
    void setAudioTimeBase(AVRational audioTimeBsse) { m_audioTimeBase = audioTimeBsse; }

    int m_fps = 0;

private:
    MediaDisplay(MediaMainControl*);
    void draw(const uint8_t *data, const int lineSize);
    void getDelay();

    ~MediaDisplay();

    static void msgOutput(const char*);

private:

    std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> m_window;
    std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> m_renderer;
    std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> m_texture;

    SDL_Rect m_windowRect;
    SDL_Thread* m_SDLEventThread = nullptr;
    PlayState m_playState;

    AudioBuffer m_audioBuffer;
    SDL_AudioSpec m_audioSpec;

    FrameQueue *m_videoFrameQueue = nullptr;
    FrameQueue *m_audioFrameQueue = nullptr;
    AVRational m_videoTimeBsse;
    AVRational m_audioTimeBase;

    MediaMainControl *m_mainControl = nullptr;
};

#endif