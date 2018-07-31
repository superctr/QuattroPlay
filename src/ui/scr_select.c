#include <string.h>

#include "../qp.h"
#include "ui.h"

#define PLPAGE (FROWS-7)
    static int select_pos;

static void select_pos_check()
{
    if(select_pos < 0)
        select_pos = 0;
    if(select_pos >= Audit->Count)
        select_pos = Audit->Count-1;
}
static void scr_select_input()
{
    got_input=0;

    int increment=1;

    switch(keycode)
    {
    case SDLK_UP:
    case SDLK_PAGEUP:
        increment=-increment;
    case SDLK_DOWN:
    case SDLK_PAGEDOWN:
        if(keycode == SDLK_PAGEUP || keycode == SDLK_PAGEDOWN)
            increment *= PLPAGE;
        select_pos  += increment;
        select_pos_check();
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        running = 0;
        returncode = select_pos+1;
        select_pos_check();
        Game->PlaylistPosition=select_pos;
        break;
    case SDLK_F3: // refresh rom defs
        if(!Audit->AuditFlag)
            Audit->Count = 0;
        break;
    default:
        break;
    }
}


void scr_select()
{
    /*
    if(refresh & R_SCR_SELECT)
    {
        refresh &= ~R_SCR_SELECT;
        select_pos=0;
    }
    */

    if(!Audit->AuditFlag && !Audit->Count)
    {
        Audit->AuditFlag = 2;
        SDL_Thread *t = NULL;
        t = SDL_CreateThread(AuditGames,"AuditGames",Audit);
        if(!t)
            AuditGames(Audit);
    }
    if(!Audit->AuditFlag && Audit->Count && !Audit->CheckCount)
    {
        Audit->AuditFlag = 1;
        SDL_Thread *t = NULL;
        t = SDL_CreateThread(AuditRoms,"AuditRoms",Audit);
        if(!t)
            AuditRoms(Audit);
    }

    set_color(1,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_50,COLOR_L_GREY);
    set_color(3,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_25,COLOR_L_GREY);
    set_color(5,1,FROWS-7,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
    set_color(49,0,1,FCOLUMNS,COLOR_D_BLUE,COLOR_L_GREY);
    SCRN(1,1,FCOLUMNS-2,QP_TITLE " Version " QP_VERSION " ("QP_WEBSITE")");

    if(Audit->AuditFlag == 2)
    {
        SCRN(3,1,FCOLUMNS-2,"Reading definitions... (%d)",Audit->Count);
        SCRN(5,1,FCOLUMNS-2,"Please wait ...");
    }
    else if(Audit->Count == -1)
    {
        SCRN(5,1,FCOLUMNS-2,"No game configs present");
    }
    else if(Audit->Count > 0)
    {
        if(Audit->AuditFlag == 1)
            SCRN(3,1,FCOLUMNS-2,"Checking ROMs... (%d of %d)",Audit->CheckCount,Audit->Count);
        else
            SCRN(3,1,FCOLUMNS-2,"Select a game (%d of %d available)",Audit->OkCount,Audit->Count);

        if(got_input)
            scr_select_input();

        int y = 0;
        int i = select_pos;
        int offset = 0;
        int max = PLPAGE;
        if(Audit->Count < max)
            max = Audit->Count;
        if(i>(PLPAGE/2) && Audit->Count > max)
            offset = i-(PLPAGE/2);
        if(offset+max > Audit->Count)
            offset -= (offset+max)-Audit->Count;

        for(y=0;y<max;y++)
        {
            i=y+offset;
            colorsel_t bg=COLOR_D_BLUE,fg=COLOR_L_GREY;

            if(select_pos == i)
            {
                bg = COLOR_N_BLUE;
                fg = COLOR_WHITE;
            }

            if(!Audit->Entry[i].RomOk)
                fg = COLOR_D_GREY;

            set_color(5+y,1,1,FCOLUMNS-2,bg,fg);

            SCRN(5+y,1,FCOLUMNS-2,"%-14s %s",Audit->Entry[i].Name,Audit->Entry[i].DisplayName);

            if(Audit->Entry[i].HasPlaylist == 2)
                set_color(5+y,FCOLUMNS-2,1,1,bg,COLOR_D_GREY);
            if(Audit->Entry[i].HasPlaylist)
                SCRN(5+y,FCOLUMNS-2,2,"P");
        }

        SCRN(49,1,48,"ENTER: load game");
    }
}
