#ifndef PMEM_USAGE_H
#define PMEM_USAGE_H

#include <stdint.h>

uint64_t pmem_usage(void);

#ifdef PMEM_USAGE_IMPLEMENTATION

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

// Temporarily rename conflicting Windows declarations
#define Rectangle    Win32Rectangle
#define CloseWindow  Win32CloseWindow
#define ShowCursor   Win32ShowCursor
#define LoadImageA   Win32LoadImageA
#define LoadImageW   Win32LoadImageW
#define DrawTextA    Win32DrawTextA
#define DrawTextW    Win32DrawTextW
#define DrawTextExA  Win32DrawTextExA
#define DrawTextExW  Win32DrawTextExW
#define PlaySoundA   Win32PlaySoundA
#define PlaySoundW   Win32PlaySoundW

#include <windows.h>
#include <psapi.h>

// Restore names before including raylib/raygui
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef LoadImageA
#undef LoadImageW
#undef DrawText
#undef DrawTextA
#undef DrawTextW
#undef DrawTextEx
#undef DrawTextExA
#undef DrawTextExW
#undef PlaySound
#undef PlaySoundA
#undef PlaySoundW

uint64_t pmem_usage(void)
{
    PROCESS_MEMORY_COUNTERS counters;

    if (!GetProcessMemoryInfo(
            GetCurrentProcess(),
            &counters,
            sizeof(counters)))
        return 0;

    return (uint64_t)counters.WorkingSetSize;
}

#elif defined(__linux__)

#include <stdio.h>

uint64_t pmem_usage(void)
{
    FILE *file;
    char line[128];
    unsigned long long memory_kb;

    file = fopen("/proc/self/status", "r");
    if (!file)
        return 0;

    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "VmRSS: %llu kB", &memory_kb) == 1)
        {
            fclose(file);
            return (uint64_t)memory_kb * 1024;
        }
    }

    fclose(file);
    return 0;
}

#else

uint64_t pmem_usage(void)
{
    return 0;
}

#endif

#endif //PMEM_USAGE_IMPLEMENTATION

#endif //PMEM_USAGE_H

