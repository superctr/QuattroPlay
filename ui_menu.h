#ifndef UI_MENU_H_INCLUDED
#define UI_MENU_H_INCLUDED

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

// Infobar
void ui_pattern_disp(int TrackNo);
void ui_info_track(int id,int ypos);
void ui_info_voice(int id,int ypos);

// Note: "Registers" are either song request, song registers, or channels.
// Draw a register value onto the screen.
void ui_drawreg(int val,int y,int x);
// Draws the key (+0/8, +1/9 etc)
void ui_drawregkey(int y);
// Draws a row of registers.
void ui_drawregrow(int y,int val);
// Draws entire screen, calls the above functions...
void ui_drawscreen();
// Handles user input and changes state as needed.
void ui_handleinput(SDL_Keysym *ks);

    // Set to 0 to terminate main loop
    int running;

    inputstate_t inpstate;

    int displaymode;
    int curr_val;         // Current selected value
    int curr_val_edit;    // Temp value used in editing.
    int curr_val_type;    // Type of current selected value (song request / register)
    int curr_val_offset;  // Offset in song req/register table.
    int blink;

    int displaysection;

    char tempstring[512];

    char loglines[16][40];

#endif // UI_MENU_H_INCLUDED
