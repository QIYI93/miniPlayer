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
    auto const file = "G:\\movie\\Ʈ����Ӱpiaohua.com�ƶ��Թ�2HD1280���庫����Ӣ˫��.mkv";
    //auto const file = "D:\\project\\ffmpeg\\media_data\\video\\Warcraft3_End.avi";
    //auto const file = "D:\\project\\ffmpeg\\media_data\\audio\\��һ���� - ͯ����.mp3";


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
