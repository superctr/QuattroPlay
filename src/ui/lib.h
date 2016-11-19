#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED

#include "stdint.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_render.h"
#include "stdio.h"
#include "ui.h"

#define SCR(_y_,_x_,str,...) sprintf(&screen.text[_y_][_x_],str , ##__VA_ARGS__)
#define SCRN(_y_,_x_,count,str,...) snprintf(&screen.text[_y_][_x_],count,str , ##__VA_ARGS__)
#define NOTICE(str,...) {snprintf(ui_notice,80,str, ##__VA_ARGS__);ui_notice_timer=UI_NOTICE_TIME;}

#define set_color ui_color

    SDL_Window *window;
    SDL_Renderer *rend;
    SDL_Texture *font;
    SDL_Texture *dispbuf;

typedef enum {
    COLOR_BLACK = 0,
    COLOR_WHITE,
    COLOR_D_GREY,
    COLOR_N_GREY,
    COLOR_L_GREY,
    COLOR_D_RED,
    COLOR_N_RED,
    COLOR_L_RED,
    COLOR_D_GREEN,
    COLOR_N_GREEN,
    COLOR_L_GREEN,
    COLOR_D_BLUE,
    COLOR_N_BLUE,
    COLOR_L_BLUE,

    CFLAG_YSHIFT_25 = 0x100,
    CFLAG_YSHIFT_50 = 0x200,
    CFLAG_YSHIFT_75 = 0x400,
    CFLAG_YSHIFT    = 0x700,

    CFLAG_KEYBOARD  = 0x800,

} colorsel_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_t;

typedef struct {
    char text[FROWS][FCOLUMNS*2];
    colorsel_t textcolor[FROWS][FCOLUMNS*2];
    colorsel_t bgcolor[FROWS][FCOLUMNS*2];
    uint8_t dirty[FROWS][FCOLUMNS*2];
    int screen_dirty;
} screen_t;

void ui_update();
void ui_refresh();
void ui_color(int y,int x,int h,int w,colorsel_t bg,colorsel_t fg);
void ui_keyboard(int y,int x,int octaves,int note);

int  ui_init();
void ui_deinit();
void ui_clear();

#endif // LIB_H_INCLUDED
