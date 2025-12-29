#include <raylib.h>
#include "camera.h"
#include <raymath.h>

struct camera_entity camera_entity_init(Vector2 screen_size)
{
    struct camera_entity new;
    new.camera.target = (Vector2){0.0f, 0.0f};
    new.camera.offset = (Vector2){screen_size.x / 2.0f, screen_size.y / 2.0f};
    new.camera.rotation = 0.0f;
    new.camera.zoom = 1.0f;
    new.shake_intensity = 0.0f;
    new.shake_duration = 0.0f;
    new.shake_timer = 0.0f;
    new.is_shaking = false;
    new.movement_bounds = (Rectangle){
        new.camera.offset.x / 2, 
        new.camera.offset.y / 2,
        new.camera.offset.x, 
        new.camera.offset.y,
    };
    return new;
}

void camera_entity_update(struct camera_entity *gcamera)
{
    Vector2 target_position = gcamera->follow_target_entity->position;

    if (target_position.x < gcamera->camera.target.x - gcamera->movement_bounds.width / 2) {
        gcamera->camera.target.x -= abs(gcamera->follow_target_entity->speed.x) + 0.2f;
    } else if (target_position.x > gcamera->camera.target.x + gcamera->movement_bounds.width / 2) {
        gcamera->camera.target.x += abs(gcamera->follow_target_entity->speed.x) + 0.2f;
    } 
    if (target_position.y < gcamera->camera.target.y - gcamera->movement_bounds.height / 2) {
        gcamera->camera.target.y -= abs(gcamera->follow_target_entity->speed.y) + 0.2f;
    } else if (target_position.y > gcamera->camera.target.y + gcamera->movement_bounds.height / 2) {
        gcamera->camera.target.y += abs(gcamera->follow_target_entity->speed.y) + 0.2f;
    } 

    if (abs(gcamera->follow_target_entity->speed.y) == 0 && gcamera->camera.target.y > target_position.y) {
        gcamera->camera.target.y -= 2;
    }
    if (abs(gcamera->follow_target_entity->speed.x) == 0 && gcamera->camera.target.y < target_position.y) {
        gcamera->camera.target.y += 2;
    }
    if (abs(gcamera->follow_target_entity->speed.x) == 0 && gcamera->camera.target.x > target_position.x) {
        gcamera->camera.target.x -= 2;
    }
    if (abs(gcamera->follow_target_entity->speed.x) == 0 && gcamera->camera.target.x < target_position.x) {
        gcamera->camera.target.x += 2;
    }

    if (gcamera->is_shaking) {
        gcamera->shake_timer -= 1;
        if (gcamera->shake_timer <= 0.0f) {
            gcamera->is_shaking = false;
            gcamera->shake_intensity = 0.0f;
        } else {
            gcamera->camera.target.x = gcamera->camera.target.x + (GetRandomValue(-1, 1) * gcamera->shake_intensity);
            gcamera->camera.target.y = gcamera->camera.target.y + (GetRandomValue(-1, 1) * gcamera->shake_intensity);
        }
    } 
}

void camera_entity_trigger_camera_shake(struct camera_entity *gcamera, float intensity, float duration)
{
    gcamera->shake_intensity = intensity;
    gcamera->shake_duration = duration;
    gcamera->shake_timer = duration;
    gcamera->is_shaking = true; // Start shaking effect
}

void camera_entity_set_target_entity(struct camera_entity *gcamera, struct entity *target_entity)
{
    gcamera->follow_target_entity = target_entity;
}
