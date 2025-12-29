#include <raylib.h>
#include "sprite.h"

struct sprite sprite_init(const char* filepath, uint32_t rows, uint32_t *framesPerRow)
{
    struct sprite new;

    new.currentFrame = 0;
    new.currentRow = 0;
    new.framesCounter = 0;
    new.framesSpeed = 8;

    new.totalRows = rows;
    new.framesPerRow = malloc(rows * sizeof(uint32_t));

    for (uint32_t i = 0; i < rows; i++) {
        new.framesPerRow[i] = framesPerRow[i];
    }

    new.spritesheet = LoadTexture(filepath);

    new.frameRec = (Rectangle) { 
        0.0f,
        (float)new.currentRow * (new.spritesheet.height / new.totalRows), 
        (float)new.spritesheet.width / new.framesPerRow[new.currentRow],
        (float)new.spritesheet.height / new.totalRows
    };

    new.scale = (Vector2){1.0f, 1.0f};
    //new.offset = (Vector2){0.0f, 0.0f};
    new.position = (Vector2) { 350.0f, 280.0f };
    new.rotation = 0.0f;

    return new;
}

void sprite_update(struct sprite *sp)
{
    sp->framesCounter++;

    if (sp->framesCounter >= (60/sp->framesSpeed))
    {
        sp->framesCounter = 0;
        sp->currentFrame++;

        if (sp->currentFrame > sp->framesPerRow[sp->currentRow] - 1) {
            sp->currentFrame = 0;
        }

        sp->frameRec.x = (float)sp->currentFrame * (float)(sp->spritesheet.width / sp->framesPerRow[sp->currentRow]);
        sp->frameRec.y = (float)sp->currentRow * (float)(sp->spritesheet.height / sp->totalRows);
    }

    Vector2 offset = {
        (sp->frameRec.width * sp->scale.x)/2.0f,
        (sp->frameRec.height * sp->scale.y)/2.0f,
    };

    Rectangle dest = (Rectangle) {
        sp->position.x - offset.x,
        sp->position.y - offset.y,
        sp->frameRec.width * sp->scale.x,
        sp->frameRec.height * sp->scale.y,
    };
    DrawTexturePro(sp->spritesheet, sp->frameRec, dest, (Vector2){0.0f, 0.0f}, sp->rotation, WHITE);
}

void sprite_set_animation_state(struct sprite* sp, uint32_t row)
{
    if (row < sp->totalRows) {
        sp->currentRow = row;
        sp->currentFrame = 0;
    }
}

void sprite_set_position(struct sprite* sp, Vector2 new_position)
{
    sp->position = new_position;
}

void sprite_flip(struct sprite* sp)
{
    sp->frameRec.width *= -1;
}

void sprite_set_rotation(struct sprite* sp, float rotation)
{
    sp->rotation = rotation;
}
