#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>
#include <stdint.h>
#include "entity.h"

#define LEVEL_MAX_CELLS 1000000u
#define LEVEL_MAX_ENTITIES 100000u
#define LEVEL_MAX_LAYERS 32u
#define LEVEL_MAX_LEVELS 32u
#define LEVEL_MAX_FILE (128u * 1024u * 1024u)

enum {
    LEVEL_BACKGROUND,
    LEVEL_GROUND,
    LEVEL_FOREGROUND,
    LEVEL_LAYER_COUNT
};

struct level_layer {
    uint32_t type;
    uint32_t visible;
};

struct level {
    uint32_t width;
    uint32_t height;
    uint32_t tile_size;
    struct level_layer *layers;
    struct entity *entities;
    uint64_t next_entity_id;
};

bool level_init(struct level *level, uint32_t width, uint32_t height, uint32_t tile_size);
bool level_add_layer(struct level *level, uint32_t type);
int level_add_entity(struct level *level, struct entity entity);
struct level *level_from_entity(const struct entity *entity);
bool level_save(const struct level *levels, const char *path);
bool level_load(struct level **levels, const char *path);
void level_free(struct level *level);

#endif
