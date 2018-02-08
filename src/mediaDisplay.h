#ifndef MEDIADISPLAY_H
#define MEDIADISPLAY_H

#include <mutex>
#include <atomic>
#include "mediaMainControl.h"

enum class DisplayType :int
{
    USING_SDL = 0,  //using SDL libraries
    USING_DIRECTX,  //using d3d9 and XAudio2 libraries
    USING_OPENGL,   //using OpenGL and XAudio2 libraries
};

enum class MsgType :int
{
    MSG_ERROR = 0,
    MSG_WARNING,
    MSG_INFO,
};

typedef struct Timebase {
    int num;
    int den;
} Timebase;

typedef struct PlayState
{
    std::atomic<bool> pause = false;
    std::atomic<bool> exit = false;
    int32_t delay;
    int32_t currentAudioTime = 0;
    int32_t currentVideoTime = 0;
    std::atomic<int>  volume;
    bool audioDisplay = false;
    bool videoDisplay = false;
}PlayState;

class MediaDisplay
{
public:
    static MediaDisplay *createDisplayInstance(MediaMainControl* mainCtrl, DisplayType type);
    static void destroyDisplayInstance(MediaDisplay*);

    virtual bool init() = 0;
    virtual bool initVideoSetting(int width, int height, const char *title) = 0;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout) = 0;

    virtual void exec() = 0;

    void setVideoTimeBase(int num, int den) { m_videoTimeBase.num = num; m_videoTimeBase.den = den; }
    void setAudioTimeBase(int num, int den) { m_audioTimeBase.num = num; m_audioTimeBase.den = den; }
    void setFps(int fps) { m_fps = fps; }
    static double r2d(Timebase r) { return r.num == 0 || r.den == 0 ? 0 : (double)r.num / (double)r.den; }

    AudioParams getAudioOutPutParams() { return m_audioParams; }

protected:
    MediaDisplay(MediaMainControl*);
    void msgOutput(MsgType type, const char*,...);

protected:
    AudioParams m_audioParams;
    Timebase m_videoTimeBase = { 0,0 };
    Timebase m_audioTimeBase = { 0,0 };
    DisplayType m_displayType;
    MediaMainControl* m_mainCtrl = nullptr;
    PlayState m_playState;
    float m_fps = 0;

    std::mutex m_mutex_msg;

protected:
    MediaDisplay() = delete;
    MediaDisplay(MediaDisplay&) = delete;
    virtual ~MediaDisplay();

};

#endif