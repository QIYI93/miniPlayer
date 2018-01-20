#include "util.h"

inline bool narrowToWide(wchar_t *wide, int wideSize, const char *buf)
{
    return !!MultiByteToWideChar(CP_ACP, 0, buf, -1, wide, wideSize);
}

inline bool WideToNarrow(char *buf, int bufSize, const wchar_t *wide)
{
    return !!WideCharToMultiByte(CP_ACP, 0, wide, -1, buf, bufSize, nullptr, nullptr);
}

inline bool WideToUTF8(const wchar_t *wide, char *utf8, int utf8Size)
{
    return !!WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, utf8Size, nullptr, nullptr);
}