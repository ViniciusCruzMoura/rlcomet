#include <raylib.h>
#include "camera.h"
#include "stb_ds.h"
#include <raymath.h>
#include <math.h>

#define CAMERA_FOLLOW_RATE 10.0f

struct camera_entity camera_entity_init(Vector2 screen_size)
{
    struct camera_entity new;
    new.camera.target = (Vector2){0.0f, 0.0f};
    new.camera.offset = (Vector2){screen_size.x / 2.0f, screen_size.y / 2.0f};
    new.camera.rotation = 0.0f;
    new.camera.zoom = 1.0f;
    new.follow_target_entity = ENTITY_NONE;
    return new;
}

void camera_entity_update(struct camera_entity *gcamera, struct entity *entities)
{
    gcamera->follow_target_entity = ENTITY_NONE;
    for (ptrdiff_t i = 0; i < arrlen(entities); ++i) {
        if (entities[i].is_active && entities[i].type == PLAYER) {
            gcamera->follow_target_entity = i;
            break;
        }
    }
    if (gcamera->follow_target_entity == ENTITY_NONE) {
        return;
    }
    const struct entity *target = &entities[gcamera->follow_target_entity];
    Vector2 position = Vector2Add(target->position, (Vector2){
        fabsf(target->sp.frame_rec.width * target->sp.scale.x) / 2.0f,
        fabsf(target->sp.frame_rec.height * target->sp.scale.y) / 2.0f
    });
    float blend = 1.0f - expf(-CAMERA_FOLLOW_RATE * GetFrameTime());
    gcamera->camera.target = Vector2Lerp(gcamera->camera.target, position, blend);
}
