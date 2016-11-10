#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

// Setting to 60 will break debugging ...
#define UI_FPS 30
// Amount of frames to sample for FPS counting
#define UI_FPS_SAMPLES 16

//#define FSIZE_X 8
//#define FSIZE_Y 8
#define FCOLUMNS 80
#define FROWS 50
    int FSIZE_X;
    int FSIZE_Y;

typedef enum {
    SCR_MAIN = 0,
    SCR_ABOUT,
    SCR_PLAYLIST
} screen_mode_t;


#include "stdint.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_render.h"
#include "../qp.h"
#include "lib.h"

    float vol;
    screen_t screen;
    screen_t last_screen;

    uint32_t lasttick;
    uint32_t frame_cnt;
    double fps_cnt;

    int debug_stat;
    int draw_count;
    int rect_count;

    int got_input;
    SDL_Keycode keycode;
    int running;

    screen_mode_t screen_mode;
    screen_mode_t last_scrmode;

void ui_main();

void scr_main();
void scr_about();
void scr_playlist();

#endif // UI_H_INCLUDED
