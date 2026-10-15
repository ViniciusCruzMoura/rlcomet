#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>
#include "stb_ds.h"
#include <raylib.h>



struct sprite {
    uint32_t texture_id;

    uint32_t total_rows;
    uint32_t current_row;
    uint32_t *frames_per_row;
    Vector2 scale;
    

    uint32_t current_frame;
    uint32_t frames_counter;
    uint32_t frames_speed;
    Rectangle frame_rec;
    Color tint;
};

struct sprite sprite_init(uint32_t texture_id, uint32_t rows, const uint32_t *frames_per_row, uint32_t frames_speed);
void sprite_set_animation_state(struct sprite* sp, uint32_t row);
bool sprite_update(struct sprite *sp);
void sprite_free(struct sprite *sp);
void sprite_draw(const struct sprite *sp, Vector2 position);
void sprite_flip(struct sprite* sp);

#endif //SPRITE_H
