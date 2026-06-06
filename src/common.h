#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

char* stringf(const char *format, ...);
uint32_t rand_between(uint32_t min, uint32_t max);

#endif //COMMON_H
