#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "entity.h"
#include "camera.h"

#define MAX_OBJECTS 16384

typedef int objid;

extern int max_obj;
extern struct entity obj[MAX_OBJECTS];
extern struct game_state g;
extern struct camera_entity c;
extern objid player;

struct game_state {
    uint32_t fps;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t current_scene;
    uint32_t display_should_close;
    bool is_paused;
    bool enable_traceback;
};

struct game_state game_init(void);
uint32_t game_update(void);
uint32_t game_key_down(void);

objid alloc_objid(int type);
void free_objid(objid id);


#endif //GAME_H
