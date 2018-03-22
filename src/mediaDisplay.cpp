#include "mediaDisplay.h"

#include <stdarg.h>
#include <iostream>
#include <string>

#include "mediaDisplay_SDL.h"
#include "mediaDisplay_D3D9.h"
#include "mediaDisply_D3D11.h"
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
    case DisplayType::USING_D3D9:
        mediaDisplay = new MediaDisplay_D3D9(mainCtrl);
        break;
    case DisplayType::USING_D3D11:
        mediaDisplay = new MediaDisplayD3D11(mainCtrl);
        break;
    case DisplayType::USING_OPENGL:
        //mediaDisplay = new mediaDisplay_OpenGL();
        break;
    default:
        break;
    }
    if(mediaDisplay->init())
        return mediaDisplay;
    
    delete mediaDisplay;
    return nullptr;
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
    case DisplayType::USING_D3D9:
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
