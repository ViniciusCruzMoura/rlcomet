#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h>

struct darray {
    void** itens;
    int used;
    int capacity;
};

void darray_append(struct darray* v, void* item);
void* darray_at(struct darray* v, int i);
void darray_remove_at(struct darray* v, int idx);

#if defined(DARRAY_IMPLEMENTATION)

#include <stdlib.h>

void darray_append(struct darray* v, void* item)
{
    if (v->used >= v->capacity) {
        v->capacity += 256;
        v->itens = realloc(v->itens, v->capacity*sizeof(*v->itens));
    }
    v->itens[v->used] = item;
    ++v->used;
}

void* darray_at(struct darray* v, int i)
{
    if (i >= 0 && i < v->used) {
        return v->itens[i];
    }
    return NULL;
}

void darray_remove_at(struct darray* v, int idx)
{
    if (idx >=0 && idx <= v->used) {
        for(int i = idx; i < v->used; ++i) {
            v->itens[i] = v->itens[i+1];
        }
        --v->used;
    }
}

#endif //DARRAY_IMPLEMENTATION

#endif //DARRAY_H
