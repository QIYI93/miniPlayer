#include "mediaDisplay.h"
#include "mediaDisplay_SDL.h"
//#include "mediaDisplay_DirectX.h"
//#include "mediaDisplay_OpenGL.h"



MediaDisplay::MediaDisplay(MediaMainControl* mainCtrl)
    :m_mainCtrl(mainCtrl)
{

}

MediaDisplay::~MediaDisplay()
{

}

MediaDisplay* MediaDisplay::createDisplayInstance(MediaMainControl* mainCtrl, DisplayType type)
{
    MediaDisplay *mediaDisplay = nullptr;
    switch (type)
    {
    case DisplayType::USING_SDL:
        //mediaDisplay = new mediaDisplay_SDL();
        break;
    case DisplayType::USING_DIRECTX:
        //mediaDisplay = new mediaDisplay_DirectX();
        break;
    case DisplayType::USING_OPENGL:
        //mediaDisplay = new mediaDisplay_OpenGL();
        break;
    default:
        break;
    }

    return mediaDisplay;
}

void MediaDisplay::destroyDisplayInstance(MediaDisplay *instance)
{
    if (instance != nullptr)
        delete instance;
}