#ifndef PMEM_USAGE_H
#define PMEM_USAGE_H

#include <stdint.h>

uint64_t pmem_usage(void);

#ifdef PMEM_USAGE_IMPLEMENTATION

#if defined(_WIN32)

#include <windows.h>
#include <psapi.h>

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

