#ifndef CAMERA_H
#define CAMERA_H

#include <raylib.h>
#include "entity.h"

struct camera_entity {
    Camera2D camera;        // raylib's 2D camera
    //Vector2 follow_speed; // Speed at which the camera follows the entity when out of bounds
    //Vector2 target_position;
    Rectangle movement_bounds; // Defines the area where the entity can move without moving the camera
    float shake_intensity;    // Intensity of the shake
    float shake_duration;     // Duration of the shake effect
    float shake_timer;        // Timer for shake duration
    bool is_shaking;          // Flag to indicate if the camera is shaking
    struct entity *follow_target_entity; // reference to the entity the camera follows
    //struct entity base; // base properties of an entity
};

struct camera_entity camera_entity_init(Vector2 screen_size);
void camera_entity_update(struct camera_entity *camera);
void camera_entity_trigger_camera_shake(struct camera_entity *gcamera, float intensity, float duration);
void camera_entity_set_target_entity(struct camera_entity *gcamera, struct entity *target_entity);

#endif //CAMERA_H
