#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>
#include <stdlib.h>
#include <raylib.h>

struct sprite {
    // SpriteSheet
    Texture2D spritesheet;        // The sprite sheet texture
    Vector2 position;             // Position of the sprite
    uint32_t totalRows;           // Total number of action rows in the sprite sheet
    uint32_t currentRow;          // Current action row
    uint32_t *framesPerRow;       // Array of frame counts for each row
    Vector2 scale;                // Scale factor (e.g., (4.0f, 4.0f) for 64x64 out of 16x16)
    //Vector2 offset;               // Offset for rendering
    float rotation;               // Rotation angle in degrees
    // SpriteAnimation
    uint32_t currentFrame;        // Current frame index
    uint32_t framesCounter;       // Counter for frame updates
    uint32_t framesSpeed;         // Speed of the animation
    Rectangle frameRec;           // Rectangle used for drawing the current frame
};

struct sprite sprite_init(const char* filepath, uint32_t rows, uint32_t *framesPerRow);
void sprite_set_animation_state(struct sprite* sp, uint32_t row);
void sprite_update(struct sprite *sp);
void sprite_set_position(struct sprite* sp, Vector2 new_position);
void sprite_set_rotation(struct sprite* sp, float rotation);
void sprite_flip(struct sprite* sp);

#endif //SPRITE_H
