#include <windows.h>

#define NarrowToWideBuf(narrowBuf, wide) narrowToWide(wide, _countof(wide), narrowBuf)
#define WideToNarrowBuf(wide, narrowBuf) WideTonarrow(narrowBuf, _countof(narrowBuf), wide)

static inline bool narrowToWide(wchar_t *wide, int wideSize, const char *buf)
{
    return !!MultiByteToWideChar(CP_ACP, 0, buf, -1, wide, wideSize);
}

static inline bool WideTonarrow(char *buf, int bufSize, const wchar_t *wide)
{
    return !!WideCharToMultiByte(CP_ACP, 0, wide, -1, buf, bufSize, nullptr, nullptr);
}