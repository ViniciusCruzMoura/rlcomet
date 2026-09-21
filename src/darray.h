#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h>

struct darray {
    void** itens;
    size_t item_size;
    int used;
    int capacity;
};

int darray_append_(struct darray* v, void* item, size_t item_size);
#define darray_append(v, item) darray_append_((v), (item), sizeof(*(item)))
void* darray_at(struct darray* v, int i);
void darray_remove_at(struct darray* v, int idx);
void darray_free(struct darray* v);

#if defined(DARRAY_IMPLEMENTATION)

#include <stdlib.h>

int darray_append_(struct darray* v, void* item, size_t item_size)
{
    if (v->item_size && v->item_size != item_size) return 0;
    v->item_size = item_size;

    if (v->used >= v->capacity) {
        v->capacity += 256;
        void** itens = realloc(v->itens, v->capacity*sizeof(*v->itens));
        if (!itens) return 0;
        v->itens = itens;
    }
    v->itens[v->used] = item;
    ++v->used;
    return 1;
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
    if (idx >= 0 && idx < v->used) {
        for(int i = idx; i < v->used - 1; ++i) {
            v->itens[i] = v->itens[i+1];
        }
        --v->used;
    }
}

void darray_free(struct darray* v)
{
    free(v->itens);
    *v = (struct darray){0};
}

#endif //DARRAY_IMPLEMENTATION

#endif //DARRAY_H
