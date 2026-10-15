#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdbool.h>
#include <stddef.h>
#include <raylib.h>

// Ordered, convex vertices in local coordinates. The polygon owns its vertices.
struct physics_polygon {
    Vector2 *vertices;
};

struct physics_polygon physics_polygon_init(const Vector2 *vertices, size_t count);
struct physics_polygon physics_polygon_rectangle(Rectangle rect);
void physics_polygon_free(struct physics_polygon *polygon);
// Returns an MTV that moves polygon a out of polygon b. Touching is not a collision.
bool physics_collision(const struct physics_polygon *a, Vector2 position_a,
                       const struct physics_polygon *b, Vector2 position_b, Vector2 *mtv);

#ifdef PHYSICS_IMPLEMENTATION

#include <float.h>
#include <math.h>
#include <raymath.h>
#include "stb_ds.h"

struct physics_polygon physics_polygon_init(const Vector2 *vertices, size_t count)
{
    struct physics_polygon polygon = {0};
    if (vertices && count >= 3) {
        arrsetlen(polygon.vertices, count);
        for (size_t i = 0; i < count; ++i) {
            polygon.vertices[i] = vertices[i];
        }
    }
    return polygon;
}

struct physics_polygon physics_polygon_rectangle(Rectangle rect)
{
    return physics_polygon_init((Vector2[]){
        {rect.x, rect.y}, {rect.x + rect.width, rect.y},
        {rect.x + rect.width, rect.y + rect.height}, {rect.x, rect.y + rect.height}
    }, 4);
}

void physics_polygon_free(struct physics_polygon *polygon)
{
    if (polygon) {
        arrfree(polygon->vertices);
        *polygon = (struct physics_polygon){0};
    }
}

static Rectangle physics_bounds(const struct physics_polygon *polygon, Vector2 position)
{
    Vector2 first = Vector2Add(polygon->vertices[0], position);
    float left = first.x, right = first.x, top = first.y, bottom = first.y;
    for (int i = 1; i < arrlen(polygon->vertices); ++i) {
        Vector2 point = Vector2Add(polygon->vertices[i], position);
        left = fminf(left, point.x);
        right = fmaxf(right, point.x);
        top = fminf(top, point.y);
        bottom = fmaxf(bottom, point.y);
    }
    return (Rectangle){left, top, right - left, bottom - top};
}

static void physics_project(const struct physics_polygon *polygon, Vector2 position,
                            Vector2 axis, float *min, float *max)
{
    *min = *max = Vector2DotProduct(Vector2Add(polygon->vertices[0], position), axis);
    for (int i = 1; i < arrlen(polygon->vertices); ++i) {
        float projected = Vector2DotProduct(Vector2Add(polygon->vertices[i], position), axis);
        *min = fminf(*min, projected);
        *max = fmaxf(*max, projected);
    }
}

bool physics_collision(const struct physics_polygon *a, Vector2 position_a,
                       const struct physics_polygon *b, Vector2 position_b, Vector2 *mtv)
{
    if (mtv) {
        *mtv = (Vector2){0};
    }
    if (!a || !b || arrlen(a->vertices) < 3 || arrlen(b->vertices) < 3 ||
        !CheckCollisionRecs(physics_bounds(a, position_a), physics_bounds(b, position_b))) {
        return false;
    }

    float shortest = FLT_MAX;
    Vector2 translation = {0};
    for (int shape = 0; shape < 2; ++shape) {
        const struct physics_polygon *polygon = shape ? b : a;
        for (int i = 0; i < arrlen(polygon->vertices); ++i) {
            Vector2 edge = Vector2Subtract(polygon->vertices[(i + 1) % arrlen(polygon->vertices)],
                                           polygon->vertices[i]);
            float length = Vector2Length(edge);
            if (length == 0) {
                continue;
            }
            Vector2 axis = (Vector2){-edge.y / length, edge.x / length};
            float min_a, max_a, min_b, max_b;
            physics_project(a, position_a, axis, &min_a, &max_a);
            physics_project(b, position_b, axis, &min_b, &max_b);
            float positive = max_b - min_a;
            float negative = max_a - min_b;
            if (positive <= 0 || negative <= 0) {
                return false;
            }
            float distance = positive < negative ? positive : -negative;
            if (fabsf(distance) < shortest) {
                shortest = fabsf(distance);
                translation = Vector2Scale(axis, distance);
            }
        }
    }
    if (shortest == FLT_MAX) {
        return false;
    }
    if (mtv) {
        *mtv = translation;
    }
    return true;
}

#endif
#endif
