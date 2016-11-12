#include "string.h"
#include "../qp.h"

#include "ui.h"

void scr_about()
{
    set_color(1,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_50,COLOR_L_GREY);
    set_color(3,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_25,COLOR_L_GREY);

    SCRN(1,1,FCOLUMNS-2,QP_TITLE " Version " QP_VERSION " ("QP_WEBSITE")");
    SCRN(3,1,FCOLUMNS-2,"Copyright " QP_COPYRIGHT "; Licensed under " QP_LICENSE);

    int y;
    int h;

    y=5;
    if(gameloaded)
    {
        h=7;
        set_color(y,1,h,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
        SCRN(y+1,2,FCOLUMNS-3,"Game settings");
        SCRN(y+3,3,FCOLUMNS-4,"%-20s%s",
             "Title", Game->Title);
        SCRN(y+4,3,FCOLUMNS-4,"%-20s%s",
             "Default output", Game->MuteRear ? "Stereo" : "Quad");
        SCRN(y+5,3,FCOLUMNS-4,"%-20s%f",
             "Gain", Game->Gain);

        y+=h+1;
        h=7;
        set_color(y,1,h,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
        SCRN(y+1,2,FCOLUMNS-3,"Sound driver settings");
        SCRN(y+3,3,FCOLUMNS-4,"%-20s%s",
             "Driver", "Quattro");
        SCRN(y+4,3,FCOLUMNS-4,"%-20s%s",
             "Driver type", Q_McuNames[QDrv->McuType] );
        SCRN(y+5,3,FCOLUMNS-4,"%-20s%s (%d Hz)",
             "Sound", "Namco C352", QDrv->ChipClock);

        y+=h+1;
        h=8;

        set_color(y,1,h,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
        SCRN(y+1,2,FCOLUMNS-3,"Audio settings");
        SCRN(y+3,3,FCOLUMNS-4,"%-20s%d",
             "Sample rate", Audio->as.freq);
        SCRN(y+4,3,FCOLUMNS-4,"%-20s%d",
             "Channels", Audio->as.channels);
        SCRN(y+5,3,FCOLUMNS-4,"%-20s%d",
             "Buffer size", Audio->as.samples);

        int af = Audio->as.format;
        static char temp[80];
        snprintf(temp,80,"%d-bit %s",SDL_AUDIO_BITSIZE(af),SDL_AUDIO_ISFLOAT(af)?"Float":"");
        SCRN(y+6,3,FCOLUMNS-4,"%-20s%s",
             "Format",temp);
    }

    if(got_input)
        screen_mode = last_scrmode;

}
