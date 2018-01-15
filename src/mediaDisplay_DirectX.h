#ifndef MediaDisplay_Directx_H
#define MediaDisplay_Directx_H

#include <memory>
#include <mutex>

#include "mediaDisplay.h"

//typedef struct PlayState
//{
//    bool pause = false;
//    bool exit = false;
//    int32_t delay;
//    int32_t currentAudioTime = 0;
//    int32_t currentVideoTime = 0;
//
//    bool audioDisplay = false;
//    bool videoDisplay = false;             //used
//    SDL_Event SDLEvent;
//}PlayState;
//
//typedef struct videoBuffer
//{
//    uint8_t *data[AV_NUM_DATA_POINTERS] = { 0 };
//    int  lineSize[AV_NUM_DATA_POINTERS] = { 0 };
//    uint32_t size;
//}videoBuffer;
//
//typedef struct AudioBuffer
//{
//    uint8_t *PCMBuffer = nullptr;
//    uint8_t *pos = 0;
//    int PCMBufferSize = 0;
//    int restSize = 0;
//    int bytesPerSec = 0;
//}AudioBuffer;

class MediaDisplay_Directx : public MediaDisplay
{
public:
    MediaDisplay_Directx(MediaMainControl *mainCtrl);

    virtual bool init() override;
    virtual bool initVideoSetting(int width, int height, const char *title) override;
    virtual bool initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout) override;

    virtual void exec() override;

private:
    MediaDisplay_Directx(MediaDisplay_Directx&) = delete;
    ~MediaDisplay_Directx();

private:

    //PlayState m_playState;
    //videoBuffer m_videoBuffer;
    //AudioBuffer m_audioBuffer;
    //bool m_quit = false;

};

#endif