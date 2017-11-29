#include <iostream>

#include "mediaMainControl.h"


#ifndef _DEBUG  
extern "C"
{
#include "SDL.h"
}
#endif 


int main(int argc, char * argv[])
{
    MediaMainControl::getInstance()->openFile("C:\\Project\\ffmpeg\\ffmpeg_study\\resource\\video\\sintel.wmv");
    MediaMainControl::getInstance()->play();

    //for (;;)
    //{
    //    std::chrono::milliseconds dura(2000);
    //    std::this_thread::sleep_for(dura);
    //}
    return 0;
}
