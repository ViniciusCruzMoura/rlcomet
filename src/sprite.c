#include <raylib.h>
#include "sprite.h"
#include "assets.h"
#include <math.h>

struct sprite sprite_init(uint32_t texture_id, uint32_t rows, const uint32_t *frames_per_row, uint32_t frames_speed)
{
    struct sprite new = {0};
    Texture2D tex = assets_texture(texture_id);

    new.texture_id = texture_id;
    new.current_frame = 0;
    new.current_row = 0;
    new.frames_counter = 0;
    new.frames_speed = frames_speed;
    new.tint = WHITE;

    new.total_rows = rows;
    if (!rows || !frames_per_row) {
        return new;
    }
    for (uint32_t i = 0; i < rows; ++i) {
        arrput(new.frames_per_row, frames_per_row[i]);
    }

    new.frame_rec = (Rectangle) { 
        0.0f,
        (float)new.current_row * (tex.height / new.total_rows), 
        (float)tex.width / new.frames_per_row[new.current_row],
        (float)tex.height / new.total_rows
    };

    new.scale = (Vector2){1.0f, 1.0f};

    return new;
}

// TODO 202609212106 add a flag have_sprite
// to avoid running this logic without a texture2d
bool sprite_update(struct sprite *sp)
{
    if (!sp->total_rows || !sp->frames_per_row || !sp->frames_speed) {
        return false;
    }
    Texture2D tex = assets_texture(sp->texture_id);
    sp->frames_counter++;

    if (sp->frames_counter >= 60 / sp->frames_speed)
    {
        sp->frames_counter = 0;
        sp->current_frame++;

        bool finished = sp->current_frame >= sp->frames_per_row[sp->current_row];
        if (finished) {
            sp->current_frame = 0;
        }
        sp->frame_rec.x = (float)sp->current_frame * (float)(tex.width / sp->frames_per_row[sp->current_row]);
        sp->frame_rec.y = (float)sp->current_row * (float)(tex.height / sp->total_rows);
        return finished;
    }
    return false;
}

void sprite_free(struct sprite *sp)
{
    arrfree(sp->frames_per_row);
    *sp = (struct sprite){0};
}

void sprite_draw(const struct sprite *sp, Vector2 position)
{
    Texture2D tex = assets_texture(sp->texture_id);
    if (!tex.id || (!sp->frame_rec.width && (!sp->total_rows || !sp->frames_per_row))) {
        return;
    }
    Rectangle source = sp->frame_rec;
    if (!source.width || !source.height) {
        source.width = (float)tex.width / sp->frames_per_row[sp->current_row];
        source.height = (float)tex.height / sp->total_rows;
        source.x = sp->current_frame * source.width;
        source.y = sp->current_row * source.height;
    }
    Rectangle dest = (Rectangle) {
        position.x,
        position.y,
        fabsf(source.width * sp->scale.x),
        fabsf(source.height * sp->scale.y),
    };
    DrawTexturePro(tex, source, dest, (Vector2){0.0f, 0.0f}, 0.0f, sp->tint);
}

void sprite_set_animation_state(struct sprite* sp, uint32_t row)
{
    if (row < sp->total_rows) {
        sp->current_row = row;
        sp->current_frame = 0;
        sp->frames_counter = 0;
        sp->frame_rec.x = 0;
        sp->frame_rec.y = row * sp->frame_rec.height;
    }
}

void sprite_flip(struct sprite* sp)
{
    sp->frame_rec.width *= -1;
}
