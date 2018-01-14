#include "mediaDisplay.h"

#include <stdarg.h>
#include <iostream>
#include <string>

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
        mediaDisplay = new MediaDisplay_SDL(mainCtrl);
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



void MediaDisplay::msgOutput(MsgType type, const char* msg, ...)
{
    std::unique_lock<std::mutex> lock(m_mutex_msg);
    std::string str1, str2;
    switch (m_displayType)
    {
    case DisplayType::USING_SDL:
        str1 = "[SDL]:";
        break;
    case DisplayType::USING_DIRECTX:
        str1 = "[DirectX]:";
        break;
    case DisplayType::USING_OPENGL:
        str1 = "[OpenGl]:";
        break;
    default:
        break;
    }
    switch (type)
    {
    case MsgType::MSG_ERROR:
        str2 = "<error> ";
        break;
    case MsgType::MSG_WARNING:
        str2 = "<warning> ";
        break;
    case MsgType::MSG_INFO:
        str2 = "<info> ";
        break;
    default:
        break;
    }

    va_list args;

    va_start(args, msg);
    char out[4096];
    vsnprintf(out, sizeof(out), msg, args);
    va_end(args);

    std::cout << str1 << str2  << out;
}
