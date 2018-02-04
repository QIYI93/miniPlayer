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
    auto const file = "G:\\movie\\飘花电影piaohua.com移动迷宫2HD1280高清韩版中英双字.mkv";
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
