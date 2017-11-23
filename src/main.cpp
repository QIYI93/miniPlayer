#include <iostream>

#include "mediaMainControl.h"

int main(int argc, char * argv[])
{
    MediaMainControl::getInstance()->openFile("D:\\project\\ffmpeg\\media_data\\video\\sintel.wmv");
    return 0;
}

