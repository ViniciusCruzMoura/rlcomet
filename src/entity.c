#include "game.h"
#include "level.h"
#include "entity.h"
#include "sprite.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <math.h>

#define BULLET_SPEED 6.0f
#define BULLET_LIFETIME 180u
#define ENEMY_STARTING_HEALTH 3

void entity_free(struct entity *o)
{
    sprite_free(&o->sp);
    physics_polygon_free(&o->collider);
    o->is_active = 0;
    o->type = NONE;
}

struct entity entity_init(uint32_t type)
{
    struct entity new = {0};
    new.type = type;
    new.is_active = true;
    new.is_visible = true;
    new.sp.tint = WHITE;
    new.position = (Vector2) {0, 0};
    new.speed = (Vector2) {1, 0};
    new.facing = (Vector2){1, 0};
    if (type == ENEMY) {
        new.is_solid = true;
        new.health = ENEMY_STARTING_HEALTH;
    }
    return new;
}

struct entity entity_copy(const struct entity *src)
{
    if (!src) { return (struct entity){0}; }
    struct entity copy = *src;
    copy.collider = physics_polygon_init(src->collider.vertices, arrlenu(src->collider.vertices));
    if (src->sp.total_rows && src->sp.frames_per_row) {
        struct sprite sp = sprite_init(src->sp.texture_id, src->sp.total_rows, src->sp.frames_per_row, src->sp.frames_speed);
        if (!sp.frames_per_row) {
            physics_polygon_free(&copy.collider);
            return (struct entity){0};
        }
        copy.sp.frames_per_row = sp.frames_per_row;
    }
    return copy;
}

struct entity entity_bullet_init(void)
{
    struct entity bullet = entity_init(BULLET);
    bullet.collider = physics_polygon_rectangle((Rectangle){0, 0, GRID_SIZE / 2, GRID_SIZE / 2});
    bullet.sp.texture_id = TEX_BULLET;
    bullet.sp.frame_rec = (Rectangle){0, 0, GRID_SIZE / 2, GRID_SIZE / 2};
    bullet.sp.scale = (Vector2){1, 1};
    bullet.lifetime = BULLET_LIFETIME;
    bullet.damage = 1;
    return bullet;
}

static bool entity_act_shoot_missle(struct entity *s)
{
    struct level *level = level_from_entity(s);
    if (!level) {
        return false;
    }
    struct entity bullet = entity_bullet_init();
    bullet.position = Vector2Add(s->position, Vector2Scale(s->facing, GRID_SIZE));
    bullet.speed = Vector2Scale(s->facing, BULLET_SPEED);
    bullet.owner_id = s->id;
    bullet.layer = s->layer;
    if (level_add_entity(level, bullet) < 0) {
        entity_free(&bullet);
        return false;
    }
    return true;
}

static void entity_projectile_update(struct entity *s)
{
    if (s->lifetime == 0) {
        entity_free(s);
        return;
    }
    --s->lifetime;
    struct level *level = level_from_entity(s);
    if (!level) {
        entity_free(s);
        return;
    }
    int steps = (int)ceilf(Vector2Length(s->speed));
    if (steps == 0) {
        steps = 1;
    }
    Vector2 step = Vector2Scale(s->speed, 1.0f / steps);
    for (int n = 0; n < steps; ++n) {
        s->position = Vector2Add(s->position, step);
        for (int i = 0; i < arrlen(level->entities); ++i) {
            struct entity *target = &level->entities[i];
            if (!target->is_active || !target->is_solid || target->id == s->owner_id || target == s) {
                continue;
            }
            if (physics_collision(&s->collider, s->position,
                                  &target->collider, target->position, NULL)) {
                if (target->health > 0) {
                    if (s->damage >= (uint32_t)target->health) {
                        entity_free(target);
                    } else {
                        target->health -= (int32_t)s->damage;
                    }
                }
                entity_free(s);
                return;
            }
        }
    }
}

static void entity_move_solid(struct entity *s, Vector2 delta)
{
    s->position = Vector2Add(s->position, delta);
    struct level *level = level_from_entity(s);
    if (!level) {
        return;
    }
    if (!arrlen(s->collider.vertices)) {
        s->collider = physics_polygon_rectangle((Rectangle){0, 0, level->tile_size, level->tile_size});
    }
    for (int i = 0; i < arrlen(level->entities); ++i) {
        struct entity *other = &level->entities[i];
        Vector2 mtv;
        if (other != s && other->is_active && other->is_solid &&
            physics_collision(&s->collider, s->position,
                              &other->collider, other->position, &mtv)) {
            s->position = Vector2Add(s->position, mtv);
        }
    }
}

void entity_update(struct entity *s)
{
    if (!s->is_active) {
        return;
    }
    if (s->damage > 0) {
        entity_projectile_update(s);
        return;
    }
    switch (s->type) {
        case PLAYER: {
            // TODO 202609212108 create a entity_player_update()
            // to handle player logics/tick
            bool shooting = IsKeyPressed(KEY_SPACE);
            s->speed = (Vector2){IsKeyDown(KEY_D) - IsKeyDown(KEY_A),
                                 IsKeyDown(KEY_S) - IsKeyDown(KEY_W)};
            if (Vector2LengthSqr(s->speed) > 0) {
                s->facing = Vector2Normalize(s->speed);
            }
            if (s->speed.x && (s->speed.x < 0) != (s->sp.frame_rec.width < 0)) {
                sprite_flip(&s->sp);
            }
            if (shooting) {
                sprite_set_animation_state(&s->sp, PLAYER_GUN_ATTACK);
            }
            if (s->sp.current_row != PLAYER_GUN_ATTACK) {
                uint32_t row = s->speed.x || s->speed.y ? PLAYER_WALK : PLAYER_IDLE;
                if (s->sp.current_row != row) {
                    sprite_set_animation_state(&s->sp, row);
                }
            }
            entity_move_solid(s, (Vector2){s->speed.x, 0});
            entity_move_solid(s, (Vector2){0, s->speed.y});
            if (sprite_update(&s->sp) && s->sp.current_row == PLAYER_GUN_ATTACK) {
                sprite_set_animation_state(&s->sp,
                    s->speed.x || s->speed.y ? PLAYER_WALK : PLAYER_IDLE);
            }
            if (shooting) {
                entity_act_shoot_missle(s);
            }
            break;
        }
        case ENEMY:
            sprite_update(&s->sp);
            break;
    }
}

void entity_draw(const struct entity *s, float opacity)
{
    if (!s->is_active || !s->is_visible) {
        return;
    }
    struct sprite sp = s->sp;
    Texture2D tex = assets_texture(sp.texture_id);
    if (!tex.id || !sp.frame_rec.width || !sp.frame_rec.height || !sp.scale.x || !sp.scale.y) {
        tex = assets_texture(TEX_NO_TEXTURE);
        if (!tex.id) {
            return;
        }
        float width = fabsf(sp.frame_rec.width * sp.scale.x);
        float height = fabsf(sp.frame_rec.height * sp.scale.y);
        sp.texture_id = TEX_NO_TEXTURE;
        sp.frame_rec = (Rectangle){0, 0, tex.width, tex.height};
        sp.scale = (Vector2){width > 0 ? width / tex.width : 1.0f,
                             height > 0 ? height / tex.height : 1.0f};
        sp.tint = WHITE;
    }
    sp.tint.a = (unsigned char)(sp.tint.a * opacity);
    sprite_draw(&sp, s->position);
}
