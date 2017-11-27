#include <iostream>

#include "mediaMainControl.h"

int main(int argc, char * argv[])
{
    //MediaMainControl::getInstance()->openFile("D:\\project\\ffmpeg\\media_data\\video\\sintel.wmv");
    //MediaMainControl::getInstance()->play();
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = 640;
    rect.h = 480;
    MediaDisplay *m_mediaDisplay = MediaDisplay::createSDLWindow("test", rect);
    m_mediaDisplay->show();
    for (;;)
    {
        std::chrono::milliseconds dura(2000);
        std::this_thread::sleep_for(dura);
    }
    return 0;
}
