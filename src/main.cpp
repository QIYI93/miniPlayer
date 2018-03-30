#include "mediaMainControl.h"


#ifndef _DEBUG  
extern "C"
{
#include "SDL.h"
}
#endif 

int main(int argc, char * argv[])
{
    if (argc <= 1)
        return -1;

    const char *file = argv[1];
    {
        MediaMainControl ctrl;
        if (ctrl.openFile(file))
        {
            ctrl.play();
            ctrl.closeFile();
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100000));
    return 0;
}
