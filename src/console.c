#include "console.h"

void console_keyboard(struct console* cmd)
{
    if (!cmd->input_active) {
        if (IsKeyPressed(KEY_K)) {
            cmd->first_row--;
        }
        if (IsKeyPressed(KEY_J)) {
            cmd->first_row++;
        }
        return;
    }

    int key;
    while ((key = GetCharPressed()) > 0) {
        int length = strlen(cmd->input);
        if (length < (int)sizeof(cmd->input) - 1 &&
            key >= 32 &&
            key <= 126) {
            cmd->input[length] = (char)key;
            cmd->input[length + 1] = '\0';
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        int length = strlen(cmd->input);
        if (length > 0) {
            cmd->input[length - 1] = '\0';
        }
    }

    if (IsKeyPressed(KEY_ENTER) && cmd->input[0] != '\0') {
        char* message = malloc(strlen(cmd->input) + 1);

        if (message == NULL) {
            return;
        }

        strcpy(message, cmd->input);
        console_text_append(&cmd->trace, LT_INFO, message);

        strcpy(cmd->command, cmd->input);

        cmd->command_ready = true;
        cmd->input[0] = '\0';
    }
}

void console_mouse(struct console* cmd)
{
    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        cmd->input_active = CheckCollisionPointRec(mouse, cmd->input_area);

        if (CheckCollisionPointRec(mouse, cmd->scrollbar_thumb)) {
            cmd->scrollbar_dragging = true;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        cmd->scrollbar_dragging = false;
    }

    if (CheckCollisionPointRec(mouse, cmd->area)) {
        cmd->first_row -= (int)GetMouseWheelMove();
    }

    if (cmd->scrollbar_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        int trace_len = (int)arrlen(cmd->trace);
        int max_first_row = trace_len > cmd->rows ? trace_len - cmd->rows : 0;

        int thumb_range = cmd->scrollbar.height - cmd->scrollbar_thumb.height;

        if (max_first_row > 0 && thumb_range > 0) {
            int y = mouse.y - cmd->scrollbar_thumb.height / 2;

            if (y < cmd->scrollbar.y) {
                y = cmd->scrollbar.y;
            }

            if (y > cmd->scrollbar.y + thumb_range) {
                y = cmd->scrollbar.y + thumb_range;
            }

            cmd->first_row = (y - cmd->scrollbar.y) * max_first_row / thumb_range;
        }
    }
}

void console_update(struct console* cmd)
{
    int trace_len = (int)arrlen(cmd->trace);
    int max_first_row = trace_len > cmd->rows ? trace_len - cmd->rows : 0;

    if (trace_len != cmd->last_used) {
        cmd->first_row = max_first_row;
        cmd->last_used = trace_len;
    }

    cmd->scrollbar_thumb = cmd->scrollbar;

    if (trace_len > cmd->rows) {
        cmd->scrollbar_thumb.height = cmd->scrollbar.height * cmd->rows / trace_len;

        if (cmd->scrollbar_thumb.height < cmd->row_height) {
            cmd->scrollbar_thumb.height = cmd->row_height;
        }

        int thumb_range = cmd->scrollbar.height - cmd->scrollbar_thumb.height;

        if (max_first_row > 0) {
            cmd->scrollbar_thumb.y = cmd->scrollbar.y + thumb_range * cmd->first_row / max_first_row;
        }
    }

    console_mouse(cmd);
    console_keyboard(cmd);

    if (cmd->first_row < 0) {
        cmd->first_row = 0;
    }

    if (cmd->first_row > max_first_row) {
        cmd->first_row = max_first_row;
    }
}

void console_draw(struct console* cmd)
{
    DrawRectangleRec(cmd->area, (Color){62, 70, 55, 255});

    for (int row = 0; row < cmd->rows; row++) {
        int index = cmd->first_row + row;

        if (index >= arrlen(cmd->trace)) {
            break;
        }

        struct console_text* log = &cmd->trace[index];

        if (log->message == NULL)
            continue;

        Color color = WHITE;
        if (log->level == LT_ERROR)
            color = RED;
        else if (log->level == LT_WARNING)
            color = YELLOW;

        DrawText(
            log->message,
            cmd->area.x + 3,
            cmd->area.y + row * cmd->row_height,
            20,
            color
        );
    }

    DrawRectangleRec(cmd->scrollbar, (Color){45, 50, 40, 255});
    DrawRectangleRec(cmd->scrollbar_thumb, (Color){150, 155, 140, 255});
    DrawRectangleRec(cmd->input_area, (Color){76, 88, 68, 255});
    DrawRectangleLinesEx(
        cmd->input_area,
        1,
        cmd->input_active ? WHITE : GRAY
    );

    DrawText(
        cmd->input,
        cmd->input_area.x + 4,
        cmd->input_area.y + 2,
        18,
        WHITE
    );

    if (cmd->input_active) {
        int cursor_x = cmd->input_area.x + 4 + MeasureText(cmd->input, 18);
        DrawText(
            "_",
            cursor_x,
            cmd->input_area.y + 2,
            18,
            WHITE
        );
    }
}

// void console_command(struct console* cmd)
// {
//     if (cmd->command_ready) {
//         // TODO 202609201449 execute command line
//         // switch(cmd->command);
//         // TODO 202609201452 clear the command line input
//         cmd->command[0] = '\0';
//         cmd->command_ready = false;
//     }
// }

void console_text_append(struct console_text **trace, int level, char* msg)
{
    if (!msg) return;

    arrput(*trace, ((struct console_text) {
        .level = level,
        .message = msg,
    }));
}

void console_free(struct console* cmd)
{
    for (ptrdiff_t i = 0; i < arrlen(cmd->trace); ++i) {
        free(cmd->trace[i].message);
    }
    arrfree(cmd->trace);
}
