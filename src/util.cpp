#include "util.h"


namespace util
{
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


    LARGE_INTEGER large_integer;
    __int64 IntStart;
    double DobDff = 0;

    inline void timerStart()
    {
        if (!DobDff)
        {
            QueryPerformanceFrequency(&large_integer);
            DobDff = large_integer.QuadPart;
        }

        QueryPerformanceCounter(&large_integer);
        IntStart = large_integer.QuadPart;
    }

    inline double timerFinish()
    {
        QueryPerformanceCounter(&large_integer);
        auto IntEnd = large_integer.QuadPart;
        return (IntEnd - IntStart) * 1000 / DobDff;
    }

    inline int getNoOfProcessors()
    {
        static SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwNumberOfProcessors;
    }

}