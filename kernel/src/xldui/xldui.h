// kernel/src/xldui/xldui.h

#ifndef XLDUI_H
#define XLDUI_H

#include <stdint.h>

// ==========================================================================
// MÓDULO DE DESENHO DE FORMAS (UI - User Interface)
// ==========================================================================

// --- MODOS DE DESENHO (FLAGS) ---
#define MODE_OUTLINE_ONLY 0b00000001
#define MODE_DASHED       0b00000010

// --- FUNÇÕES DE DESENHO UNIFICADAS ---

void ui_draw_rect(int x, int y, int width, int height, uint32_t color, int mode);

// AQUI ESTÁ A LINHA QUE FALTAVA! A DECLARAÇÃO DO CÍRCULO!
void ui_draw_circle(int xc, int yc, int radius, uint32_t color, int mode);

// ==========================================================================
// MÓDULO DE CONSOLE DE TEXTO (Terminal)
// ==========================================================================

// --- FUNÇÕES DO CONSOLE ---
void console_init(uint32_t bg_color, uint32_t fg_color);
void console_clear(uint32_t color);
void console_set_color(uint32_t color);
void console_print(const char *str);
void console_get_cursor(int *x, int *y);
void console_move_cursor_back(int *x, int *y);
#endif // XLDUI_H

