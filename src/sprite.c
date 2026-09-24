#include <raylib.h>
#include "sprite.h"
#include "assets.h"

struct sprite sprite_init(uint32_t texture_id, uint32_t rows, const uint32_t *frames_per_row)
{
    struct sprite new = {0};
    Texture2D tex = assets_texture(texture_id);

    new.texture_id = texture_id;
    new.current_frame = 0;
    new.current_row = 0;
    new.frames_counter = 0;
    new.frames_speed = 8;

    new.total_rows = rows;
    new.frames_per_row = malloc(rows * sizeof(uint32_t));

    for (uint32_t i = 0; i < rows; i++) {
        new.frames_per_row[i] = frames_per_row[i];
    }

    new.frame_rec = (Rectangle) { 
        0.0f,
        (float)new.current_row * (tex.height / new.total_rows), 
        (float)tex.width / new.frames_per_row[new.current_row],
        (float)tex.height / new.total_rows
    };

    new.scale = (Vector2){1.0f, 1.0f};
    new.position = (Vector2) { 350.0f, 280.0f };
    new.rotation = 0.0f;

    return new;
}

// TODO 202609212106 add a flag have_sprite
// to avoid running this logic without a texture2d
void sprite_update(struct sprite *sp)
{
    Texture2D tex = assets_texture(sp->texture_id);

    sp->frames_counter++;

    if ( sp->frames_speed > 0 && sp->frames_counter >= (60/sp->frames_speed))
    {
        sp->frames_counter = 0;
        sp->current_frame++;

        if (sp->current_frame > sp->frames_per_row[sp->current_row] - 1) {
            sp->current_frame = 0;
        }

        sp->frame_rec.x = (float)sp->current_frame * (float)(tex.width / sp->frames_per_row[sp->current_row]);
        sp->frame_rec.y = (float)sp->current_row * (float)(tex.height / sp->total_rows);
    }

    Vector2 offset = {
        (sp->frame_rec.width * sp->scale.x)/2.0f,
        (sp->frame_rec.height * sp->scale.y)/2.0f,
    };

    Rectangle dest = (Rectangle) {
        sp->position.x - offset.x,
        sp->position.y - offset.y,
        sp->frame_rec.width * sp->scale.x,
        sp->frame_rec.height * sp->scale.y,
    };
    DrawTexturePro(tex, sp->frame_rec, dest, (Vector2){0.0f, 0.0f}, sp->rotation, WHITE);
}

void sprite_set_animation_state(struct sprite* sp, uint32_t row)
{
    if (row < sp->total_rows) {
        sp->current_row = row;
        sp->current_frame = 0;
        sp->frames_counter = 0;
    }
}

void sprite_set_position(struct sprite* sp, Vector2 new_position)
{
    sp->position = new_position;
}

void sprite_flip(struct sprite* sp)
{
    sp->frame_rec.width *= -1;
}

void sprite_set_rotation(struct sprite* sp, float rotation)
{
    sp->rotation = rotation;
}
