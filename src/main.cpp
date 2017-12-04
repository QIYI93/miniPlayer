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
    MediaMainControl::getInstance()->openFile("D:\\Ñ¸À×ÏÂÔØ\\movie\\Game.of.Thrones.S07E07.The.Dragon.and.the.Wolf.720p.AMZN.WEB-DL.DDP5.1.H.264-GoT.mkv");
    //MediaMainControl::getInstance()->openFile("D:\\project\\ffmpeg\\media_data\\video\\sintel.wmv");
    MediaMainControl::getInstance()->play();

    return 0;
}
