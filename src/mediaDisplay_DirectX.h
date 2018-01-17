#ifndef MediaDisplay_Directx_H
#define MediaDisplay_Directx_H

#include <memory>
#include <mutex>
#include "DirectX_SDK/d3d9.h"

#pragma comment (lib,"d3d9.lib")

#include "mediaDisplay.h"

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
    HWND m_mainWnd = NULL;

    //PlayState m_playState;
    //videoBuffer m_videoBuffer;
    //AudioBuffer m_audioBuffer;
    //bool m_quit = false;

};

#endif