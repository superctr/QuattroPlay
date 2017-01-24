#ifndef SCR_MAIN_H_INCLUDED
#define SCR_MAIN_H_INCLUDED
#include "../lib/q_pattern.h"

typedef enum {
    STATE_MAIN,
    STATE_SETVALUE,
} inputstate_t;

enum {
    ENTRY_UNSUPPORTED,
    ENTRY_SONGREQ,
    ENTRY_REGISTER,
    ENTRY_VOICE,
};

enum {
    SCREEN_MAIN = 0,
    SCREEN_PLAYLIST,
};

struct QP_Pattern pattern;

// Draws entire screen, calls the above functions...
void ui_info_track(int id,int ypos);
void ui_info_voice(int id,int ypos);

// driver specific
void ui_info_q_track(int id,int ypos);
void ui_info_q_voice(int id,int ypos);
void ui_info_s2_track(int id,int ypos);

    int displaysection;

#endif // SCR_MAIN_H_INCLUDED
