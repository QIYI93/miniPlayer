#include <iostream>

#include "mediaMainControl.h"

int main(int argc, char * argv[])
{
    MediaMainControl::getInstance()->openFile("D:\\project\\ffmpeg\\media_data\\video\\sintel.wmv");
    MediaMainControl::getInstance()->play();

    //for (;;)
    //{
    //    std::chrono::milliseconds dura(2000);
    //    std::this_thread::sleep_for(dura);
    //}
    return 0;
}
