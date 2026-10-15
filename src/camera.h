#ifndef CAMERA_H
#define CAMERA_H

#include <raylib.h>
#include "entity.h"

struct camera_entity {
    Camera2D camera;        // raylib's 2D camera
    entity_id follow_target_entity; // current index in the level entity array
};

struct camera_entity camera_entity_init(Vector2 screen_size);
void camera_entity_update(struct camera_entity *camera, struct entity *entities);

#endif //CAMERA_H
