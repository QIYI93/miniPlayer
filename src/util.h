#include <windows.h>

#define NarrowToWideBuf(narrowBuf, wide) narrowToWide(wide, _countof(wide), narrowBuf)
#define WideToNarrowBuf(wide, narrowBuf) WideToNarrow(narrowBuf, _countof(narrowBuf), wide)
#define WideToUTF8Buf(wide, UTF8Buf) WideToUTF8(wide, UTF8Buf, _countof(UTF8Buf))

extern bool narrowToWide(wchar_t *wide, int wideSize, const char *buf);
extern bool WideToNarrow(char *buf, int bufSize, const wchar_t *wide);
extern bool WideToUTF8(const wchar_t *wide, char *utf8, int utf8Size);