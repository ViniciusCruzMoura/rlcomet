#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>
#include <stdlib.h>
#include <raylib.h>

struct sprite {
    uint32_t texture_id;

    Vector2 position;
    uint32_t total_rows;
    uint32_t current_row;
    uint32_t *frames_per_row;
    Vector2 scale;
    
    float rotation;

    uint32_t current_frame;
    uint32_t frames_counter;
    uint32_t frames_speed;
    Rectangle frame_rec;
};

struct sprite sprite_init(uint32_t texture_id, uint32_t rows, const uint32_t *frames_per_row);
void sprite_set_animation_state(struct sprite* sp, uint32_t row);
void sprite_update(struct sprite *sp);
void sprite_set_position(struct sprite* sp, Vector2 new_position);
void sprite_set_rotation(struct sprite* sp, float rotation);
void sprite_flip(struct sprite* sp);

#endif //SPRITE_H
