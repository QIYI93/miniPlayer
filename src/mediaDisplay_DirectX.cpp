#include "MediaDisplay_Directx.h"

#include <iostream>

extern "C"
{
//#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

MediaDisplay_Directx::MediaDisplay_Directx(MediaMainControl *mainCtrl)
    :MediaDisplay(mainCtrl)
{
    m_displayType = DisplayType::USING_DIRECTX;
}

MediaDisplay_Directx::~MediaDisplay_Directx()
{
}

bool MediaDisplay_Directx::init()
{
    return true;
}


bool MediaDisplay_Directx::initVideoSetting(int width, int height, const char *title)
{
    return true;
}


bool MediaDisplay_Directx::initAudioSetting(int freq, uint8_t wantedChannels, uint32_t wantedChannelLayout)
{
    return true;
}


void MediaDisplay_Directx::exec()
{
}