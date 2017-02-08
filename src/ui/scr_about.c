#include <string.h>

#include "../qp.h"
#include "ui.h"

void scr_about()
{
    set_color(1,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_50,COLOR_L_GREY);
    set_color(3,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_25,COLOR_L_GREY);

    SCRN(1,1,FCOLUMNS-2,QP_TITLE " Version " QP_VERSION " ("QP_WEBSITE")");
    SCRN(3,1,FCOLUMNS-2,"Copyright " QP_COPYRIGHT "; Licensed under " QP_LICENSE);

    int x,y,h,i;

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
        h=8;
        set_color(y,1,h,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
        SCRN(y+1,2,FCOLUMNS-3,"Sound driver settings");
        SCRN(y+3,3,FCOLUMNS-4,"%-20s%s",
             "Driver", DriverInterface->Name);
        SCRN(y+3,3,FCOLUMNS-4,"%-20s%s",
             "Driver type",DriverGetDriverInfo());
        SCRN(y+4,3,FCOLUMNS-4,"%-20s%.0f Hz",
             "Tick rate", DriverGetTickRate());
        SCRN(y+5,3,FCOLUMNS-4,"%-20s%.0f Hz",
             "Chip rate", DriverGetChipRate());
/*
        SCRN(y+4,3,FCOLUMNS-4,"%-20s%s",
             "Driver type", Q_McuNames[QDrv->McuType] );
        SCRN(y+5,3,FCOLUMNS-4,"%-20s%s (%d Hz)",
             "Sound", "Namco C352", QDrv->ChipClock);
*/

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

        if(got_input)
            screen_mode = last_scrmode;
    }
    else
    {
        h = y+1;
        int max=SDL_GetNumAudioDevices(0);
        SCRN(h++,2,FCOLUMNS-3,"Available audio devices:");
        h++;

        static int cursor;
        int listy=h;
        for(i=-1;i<max;i++)
        {
            if(i<0)
                x = SCRN(h++,3,FCOLUMNS-4,"default");
            else
                x = SCRN(h++,3,FCOLUMNS-4,"%d: \"%s\"",i,SDL_GetAudioDeviceName(i,0));

            if((i==-1 && !strlen(Game->AudioDevice)) || (i!=-1 && !strcmp(Game->AudioDevice,SDL_GetAudioDeviceName(i,0))))
                SCRN(h-1,3+x,FCOLUMNS-4," (selected)");
        }
        h++;
        SCRN(h++,2,FCOLUMNS-3,"press ENTER to select an audio device and return");

        if(refresh & SCR_ABOUT)
        {
            refresh &= ~(SCR_ABOUT);
            cursor=-1;
        }

        if(got_input)
        {
            switch(keycode)
            {
            case SDLK_UP:
                if(cursor >= 0)
                    cursor--;
                break;
            case SDLK_DOWN:
                if(cursor < max-1)
                    cursor++;
                break;
            case SDLK_RETURN:
                if(cursor<0)
                    strncpy(Game->AudioDevice,"",sizeof(Game->AudioDevice));
                else if(cursor<max)
                    strncpy(Game->AudioDevice,SDL_GetAudioDeviceName(cursor,0),sizeof(Game->AudioDevice));
            case SDLK_ESCAPE:
                screen_mode = last_scrmode;
                break;
            }
        }

        set_color(y,1,1+h-y,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
        set_color(listy+cursor+1,1,1,FCOLUMNS-2,COLOR_N_BLUE,COLOR_L_GREY);
    }
}
