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
        int max_first_row = cmd->trace.used > cmd->rows ? cmd->trace.used - cmd->rows : 0;

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
    int max_first_row = cmd->trace.used > cmd->rows ? cmd->trace.used - cmd->rows : 0;

    if (cmd->trace.used != cmd->last_used) {
        cmd->first_row = max_first_row;
        cmd->last_used = cmd->trace.used;
    }

    cmd->scrollbar_thumb = cmd->scrollbar;

    if (cmd->trace.used > cmd->rows) {
        cmd->scrollbar_thumb.height = cmd->scrollbar.height * cmd->rows / cmd->trace.used;

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

        if (index >= cmd->trace.used) {
            break;
        }

        struct console_text* log = darray_at(&cmd->trace, index);

        if (log == NULL || log->message == NULL)
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
            18,
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

void console_text_append(void* trace, int level, char* msg)
{
    struct console_text* log = malloc(sizeof(struct console_text));
    log->level = level;
    log->message = msg;
    darray_append(trace, log);
}

// void console_update(struct console* cmd)
// {
//     const int rows = 20;
//     const int row_height = 20;
//     const int console_x = 1;
//     const int console_y = 20;
//     const int console_w = 400;
//     const int scrollbar_w = 10;
//     const int scrollbar_x = console_x + console_w - scrollbar_w;
//     const int console_h = rows * row_height;
//     const int input_y = console_y + console_h;
// 
//     if (cmd->trace.itens == NULL)
//         return;
// 
//     int max_first_row = cmd->trace.used > rows ? cmd->trace.used - rows : 0;
// 
//     /*
//         Follow new messages automatically.
//     */
//     if (cmd->trace.used != cmd->last_used) {
//         cmd->first_row = max_first_row;
//         cmd->last_used = cmd->trace.used;
//     }
// 
//     Vector2 mouse = GetMousePosition();
// 
//     /*
//         Input field focus.
//     */
//     if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
//         cmd->input_active =
//             mouse.x >= console_x &&
//             mouse.x < console_w &&
//             mouse.y >= input_y &&
//             mouse.y < input_y + row_height + 4;
//     }
// 
//     /*
//         J = scroll down
//         K = scroll up
//     */
//     if (!cmd->input_active) {
//         if (IsKeyPressed(KEY_J))
//             cmd->first_row++;
// 
//         if (IsKeyPressed(KEY_K))
//             cmd->first_row--;
//     }
// 
//     /*
//         Mouse-wheel scrolling.
//     */
//     if (mouse.x >= console_x &&
//         mouse.x < console_w &&
//         mouse.y >= console_y &&
//         mouse.y < input_y) {
//         cmd->first_row -= (int)GetMouseWheelMove();
//     }
// 
//     if (cmd->first_row < 0)
//         cmd->first_row = 0;
// 
//     if (cmd->first_row > max_first_row)
//         cmd->first_row = max_first_row;
// 
//     /*
//         Keyboard text input.
//     */
//     if (cmd->input_active) {
//         int key;
// 
//         while ((key = GetCharPressed()) > 0) {
//             int length = strlen(cmd->input);
// 
//             if (length < (int)sizeof(cmd->input) - 1 &&
//                 key >= 32 &&
//                 key <= 126) {
//                 cmd->input[length] = (char)key;
//                 cmd->input[length + 1] = '\0';
//             }
//         }
// 
//         if (IsKeyPressed(KEY_BACKSPACE)) {
//             int length = strlen(cmd->input);
// 
//             if (length > 0)
//                 cmd->input[length - 1] = '\0';
//         }
// 
//         /*
//             Add the typed text to the cmd->trace.
//         */
//         if (IsKeyPressed(KEY_ENTER) && cmd->input[0] != '\0') {
//             struct console_text* log = malloc(sizeof(struct console_text));
// 
//             if (log != NULL) {
//                 log->level = LT_INFO;
//                 log->message = malloc(strlen(cmd->input) + 1);
// 
//                 if (log->message != NULL) {
//                     strcpy(log->message, cmd->input);
//                     darray_append(&cmd->trace, log);
// 
//                     cmd->input[0] = '\0';
//                     cmd->last_used = -1;
//                 } else {
//                     free(log);
//                 }
//             }
//         }
//     }
// 
//     /*
//         Console background.
//     */
//     DrawRectangle(
//         console_x,
//         console_y,
//         console_w,
//         console_h,
//         (Color){62, 70, 55, 255}
//     );
// 
//     /*
//         Visible trace messages.
//     */
//     for (int row = 0; row < rows; row++) {
//         int index = cmd->first_row + row;
// 
//         if (index >= cmd->trace.used)
//             break;
// 
//         struct console_text* log = darray_at(&cmd->trace, index);
// 
//         if (log == NULL || log->message == NULL)
//             continue;
// 
//         Color color = WHITE;
// 
//         if (log->level == LT_ERROR)
//             color = RED;
//         else if (log->level == LT_WARNING)
//             color = YELLOW;
// 
//         DrawText(
//             log->message,
//             console_x + 3,
//             console_y + row * row_height,
//             18,
//             color
//         );
//     }
// 
//     /*
//         Scrollbar.
//     */
//     DrawRectangle(
//         scrollbar_x,
//         console_y,
//         scrollbar_w,
//         console_h,
//         (Color){45, 50, 40, 255}
//     );
// 
//     if (cmd->trace.used > rows) {
//         int thumb_h = console_h * rows / cmd->trace.used;
// 
//         if (thumb_h < row_height)
//             thumb_h = row_height;
// 
//         int thumb_range = console_h - thumb_h;
//         int thumb_y = console_y;
// 
//         if (max_first_row > 0)
//             thumb_y += thumb_range * cmd->first_row / max_first_row;
// 
//         /*
//             Start dragging the scrollbar.
//         */
//         if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
//             mouse.x >= scrollbar_x &&
//             mouse.x <= scrollbar_x + scrollbar_w &&
//             mouse.y >= thumb_y &&
//             mouse.y <= thumb_y + thumb_h) {
//             cmd->scrollbar_dragging = true;
//         }
// 
//         if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
//             cmd->scrollbar_dragging = false;
// 
//         /*
//             Drag scrollbar thumb.
//         */
//         if (cmd->scrollbar_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
//             int new_thumb_y = mouse.y - thumb_h / 2;
// 
//             if (new_thumb_y < console_y)
//                 new_thumb_y = console_y;
// 
//             if (new_thumb_y > console_y + thumb_range)
//                 new_thumb_y = console_y + thumb_range;
// 
//             cmd->first_row =
//                 (new_thumb_y - console_y) * max_first_row / thumb_range;
//         }
// 
//         DrawRectangle(
//             scrollbar_x,
//             thumb_y,
//             scrollbar_w,
//             thumb_h,
//             (Color){150, 155, 140, 255}
//         );
//     }
// 
//     /*
//         Text input field.
//     */
//     DrawRectangle(
//         console_x,
//         input_y,
//         console_w,
//         row_height + 4,
//         (Color){76, 88, 68, 255}
//     );
// 
//     DrawRectangleLines(
//         console_x,
//         input_y,
//         console_w,
//         row_height + 4,
//         cmd->input_active ? WHITE : GRAY
//     );
// 
//     DrawText(cmd->input, console_x + 4, input_y + 2, 18, WHITE);
// 
//     if (cmd->input_active) {
//         int cursor_x = console_x + 4 + MeasureText(cmd->input, 18);
//         DrawText("_", cursor_x, input_y + 2, 18, WHITE);
//     }
// }
