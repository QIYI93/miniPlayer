#ifndef MEDIADISPLAY_H
#define MEDIADISPLAY_H

#include <memory>
#include <mutex>


#include "mediaMainControl.h"

enum class DisplayType :int
{
    USING_SDL = 0,  //using SDL libraries
    USING_DIRECTX,  //using d3d9 and XAudio2 libraries
    USING_OPENGL,   //using OpenGL and XAudio2 libraries
};

typedef struct Timebase {
    int num;
    int den;
} Timebase;

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channelLayout;
    enum AVSampleFormat fmt;
    int frameSize;
    int bytesPerSec;
} AudioParams;

class MediaDisplay
{
public:
    static MediaDisplay *createDisplayInstance(MediaMainControl* mainCtrl, DisplayType type);
    static void destroyDisplayInstance(MediaDisplay*);

    virtual bool initVideoSetting(int width, int height, const char *title) = 0;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint64_t wantedChannelLayout, uint64_t sample) = 0;
    virtual void exec() = 0;


    void setVideoTimeBase(int num, int den) { m_videoTimeBsse.num = num; m_videoTimeBsse.den = den; }
    void setAudioTimeBase(int num, int den) { m_audioTimeBase.num = num; m_audioTimeBase.den = den; }
    void setFps(int fps) { m_fps = fps; }

    AudioParams getAudioOutPutParams() { return m_audioParams; }

protected:
    //static void msgOutput(const char*);

protected:
    AudioParams m_audioParams;
    Timebase m_videoTimeBsse = { 0,0 };
    Timebase m_audioTimeBase = { 0,0 };
    MediaMainControl* m_mainCtrl = nullptr;
    float m_fps = 0;


private:
    MediaDisplay();
    MediaDisplay(MediaMainControl*) = delete;
    virtual ~MediaDisplay();

};

#endif