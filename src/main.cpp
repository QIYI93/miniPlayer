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
    //auto const file = "D:\\project\\ffmpeg\\media_data\\video\\sintel.wmv";
    auto const file = "D:\\迅雷下载\\movie\\Game.of.Thrones.S07E07.The.Dragon.and.the.Wolf.720p.AMZN.WEB-DL.DDP5.1.H.264-GoT.mkv";
    //auto const file = "D:\\project\\ffmpeg\\media_data\\video\\Warcraft3_End.avi";

    //auto const file = "D:\\project\\ffmpeg\\media_data\\audio\\陈一发儿 - 童话镇.mp3";
    MediaMainControl::getInstance()->openFile(file);
    MediaMainControl::getInstance()->play();
    MediaMainControl::getInstance()->closeFile();

    for (;;)
    {
        std::chrono::milliseconds dura(2000);
        std::this_thread::sleep_for(dura);
    }

    return 0;
}
