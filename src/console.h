#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "stb_ds.h"

enum {
    LT_ERROR,
    LT_INFO,
    LT_WARNING,
};

struct console_text {
    int level;
    char* message;
};

struct console {
    struct console_text *trace;

    char input[256];
    char command[256];

    Rectangle area;
    Rectangle input_area;
    Rectangle scrollbar;
    Rectangle scrollbar_thumb;

    int rows;
    int row_height;
    int first_row;
    int last_used;

    bool input_active;
    bool scrollbar_dragging;
    bool command_ready;
};

void console_update(struct console* cmd);
void console_draw(struct console* cmd);
void console_text_append(struct console_text **trace, int level, char* msg);
void console_keyboard(struct console* cmd);
void console_mouse(struct console* cmd);
void console_free(struct console* cmd);

#endif //CONSOLE_H
