#ifndef COLLISION_H
#define COLLISION_H

#include <stdint.h>
#include <stdlib.h>
#include <raylib.h>

struct collision {
    Rectangle box; // Position of the collision box and Size of the collision box
};

struct gun_attack_collision {
    struct collision bounds; // Collision box specifically for gun attacks
    float bullet_speed;       // Speed of bullets, if applicable
    uint32_t damage;              // Damage dealt by this gun attacks
};

struct hand_attack_collision {
    struct collision bounds; // Collision box specifically for hand attacks
    uint32_t damage;              // Damage dealt by hand attacks
};

int entity_handle_gun_attack_collision(struct entity gun_bounds, struct entity target);
int entity_handle_hand_attack_collision(struct entity hand_bounds, struct entity target);

#endif //COLLISION_H
