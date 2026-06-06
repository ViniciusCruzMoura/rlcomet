#include "common.h"

char* stringf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args);
    if (size < 0) {
        va_end(args);
        return NULL;
    }
    size++;
    char* buffer = malloc(size);
    if (!buffer) {
        va_end(args);
        return NULL;
    }
    va_end(args);

    va_start(args, format);
    vsnprintf(buffer, size, format, args);
    va_end(args);
    return buffer;
}

uint32_t rand_between(uint32_t min, uint32_t max) {
    return (uint32_t)(rand() % (max - min + 1)) + min;
}

