#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct { char *p; size_t n, cap; } sb;

sb sb_new(void);
uint32_t sb_add(sb *s, const char *fmt, ...);
char *sb_cstr(sb *s);
uint32_t sb_replace(sb *s, const char *old, const char *new);
void sb_free(sb *s);
char *sb_stringf(const char *fmt, ...); //memory leak

#ifdef STRING_BUILDER_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

sb sb_new(void) {
    return (sb){0};
}

static uint32_t sb_reserve(sb *s, size_t extra) {
    size_t need = s->n + extra + 1;
    if (need <= s->cap) return 1;

    size_t cap = s->cap ? s->cap * 2 : 32;
    while (cap < need) cap *= 2;

    char *p = realloc(s->p, cap);
    if (!p) return 0;

    s->p = p;
    s->cap = cap;
    return 1;
}

uint32_t sb_add(sb *s, const char *fmt, ...) {
    va_list a, b;
    va_start(a, fmt);
    va_copy(b, a);

    uint32_t n = (uint32_t)vsnprintf(NULL, 0, fmt, a);
    va_end(a);

    if ((int32_t)n < 0 || !sb_reserve(s, n)) {
        va_end(b);
        return 0;
    }

    vsnprintf(s->p + s->n, n + 1, fmt, b);
    va_end(b);

    s->n += n;
    return 1;
}

char *sb_cstr(sb *s) {
    if (!s->p && !sb_reserve(s, 0)) return NULL;
    s->p[s->n] = 0;
    return s->p;
}

uint32_t sb_replace(sb *s, const char *old, const char *new) {
    size_t old_n, new_n, n = 0, part;
    const char *p, *q;
    char *r, *out;

    if (!s || !s->p || !old || !new) return 0;

    old_n = strlen(old);
    new_n = strlen(new);

    if (!old_n) return 1;

    p = s->p;

    while ((q = strstr(p, old))) {
        n += (size_t)(q - p) + new_n;
        p = q + old_n;
    }

    n += strlen(p);

    out = malloc(n + 1);
    if (!out) return 0;

    p = s->p;
    r = out;

    while ((q = strstr(p, old))) {
        part = (size_t)(q - p);
        memcpy(r, p, part);
        r += part;
        memcpy(r, new, new_n);
        r += new_n;
        p = q + old_n;
    }

    strcpy(r, p);

    free(s->p);
    s->p = out;
    s->n = n;
    s->cap = n + 1;

    return 1;
}

void sb_free(sb *s) {
    free(s->p);
    *s = (sb){0};
}

char *sb_stringf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
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

    va_start(args, fmt);
    vsnprintf(buffer, size, fmt, args);
    va_end(args);
    return buffer;
}

#endif //STRING_BUILDER_IMPLEMENTATION

#endif //STRING_BUILDER_H
