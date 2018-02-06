#ifndef UTIL_H
#define UTIL_H

#include <windows.h>

namespace util
{
//Character operation
#define NarrowToWideBuf(narrowBuf, wide) narrowToWide(wide, _countof(wide), narrowBuf)
#define WideToNarrowBuf(wide, narrowBuf) WideToNarrow(narrowBuf, _countof(narrowBuf), wide)
#define WideToUTF8Buf(wide, UTF8Buf) WideToUTF8(wide, UTF8Buf, _countof(UTF8Buf))

extern bool narrowToWide(wchar_t *wide, int wideSize, const char *buf);
extern bool WideToNarrow(char *buf, int bufSize, const wchar_t *wide);
extern bool WideToUTF8(const wchar_t *wide, char *utf8, int utf8Size);


//Free array
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif


//Timer
extern void   timerStart();
extern double timerFinish();

//Get Processor core number
extern int getNoOfProcessors();

}
#endif