/*
    driver independent main screen

    to be used as a fallback when no appropritate driver specific screen is available
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "../qp.h"
#include "ui.h"

#include "scr_main.h"

#define PLPAGE (FROWS-7)

// order in the enum will affect display
enum {
    ITEM_SONGREQ,
    ITEM_PARAMETER,
    ITEM_VOICE,
    ITEM_TYPE_COUNT
};

static const char* item_type_name[ITEM_TYPE_COUNT] = {
    "Song Request",
    "Parameter",
    "Voice"
};

struct main2_item {
    int type;
    uint16_t index;
};

    static struct main2_item item[1024];

    static char tempstring[1024];

    int item_cnt;

    static int select_pos;
    static int edit_mode; // if set, item is being manipulated
    static int edit_value; // current edit value
    static int blink;

static void select_pos_check()
{
    if(select_pos < 0)
        select_pos = 0;
    if(select_pos >= item_cnt)
        select_pos = item_cnt-1;
}

static void generate_items()
{
    int item_index[ITEM_TYPE_COUNT] = {0};
    int item_max[ITEM_TYPE_COUNT] = {0};
    int item_type_max = 0;

    item_max[ITEM_SONGREQ] = DriverGetSlotCount();
    item_max[ITEM_PARAMETER] = DriverGetParameterCount();
    item_max[ITEM_VOICE] = 32;

    item_cnt=0;
    while(item_cnt<1000)
    {
        if(item_index[item_type_max] >= item_max[item_type_max])
            item_type_max++;
        else
        {
            item[item_cnt].type = item_type_max;
            item[item_cnt++].index = item_index[item_type_max]++;
        }
        if(item_type_max >= ITEM_TYPE_COUNT)
            break;
    }
}

static int item_can_be_edited(struct main2_item *i)
{
    switch(i->type)
    {
    case ITEM_SONGREQ:
    case ITEM_PARAMETER:
        return 1;
    default:
        return 0;
    }
}
static int item_check_overflow(struct main2_item *i,int value)
{
    switch(i->type)
    {
    case ITEM_SONGREQ:
        return (value >= DriverGetSongCount(i->index));
    case ITEM_PARAMETER:
        return (value > 0xffff); // temporary
    default:
        return 0;
    }
}
static int item_set_overflow(struct main2_item *i,int value)
{
    int max;
    switch(i->type)
    {
    case ITEM_SONGREQ:
        if(value<0) value=0;
        max = DriverGetSongCount(i->index);
        return (value >= max) ? max-1 : value;
    case ITEM_PARAMETER:
        return (value & 0xffff); // temporary
    default:
        return 0;
    }
}
static int item_get_value(struct main2_item *i)
{
    switch(i->type)
    {
    case ITEM_SONGREQ:
        return DriverGetSongId(i->index);
    case ITEM_PARAMETER:
        return DriverGetParameter(i->index);
    default:
        return 0;
    }
}
static void item_set_value(struct main2_item *i,int value)
{
    if(item_check_overflow(i,value))
        return;
    switch(i->type)
    {
    case ITEM_SONGREQ:
        Game->PlaylistControl = 0;
        DriverResetLoopCount();
        return DriverRequestSong(i->index,value);
    case ITEM_PARAMETER:
        return DriverSetParameter(i->index,value);
    default:
        break;
    }
}

static char* item_display_name(struct main2_item *i,char* buffer,int len)
{
    switch(i->type)
    {
    case ITEM_PARAMETER:
        if(DriverGetParameterName(i->index,buffer,len))
            break;
    default:
        snprintf(buffer,len,"%s %02x",item_type_name[i->type],i->index);
        break;
    }
    return buffer;
}
static char* item_display_value(struct main2_item *i,char* buffer,int len)
{
    double songtime;
    strncpy(buffer,"",len);
    switch(i->type)
    {
    case ITEM_VOICE:
        if(DriverGetSolo()>>i->index & 1)
            snprintf(buffer,len,"%s (Solo)",buffer);
        if(DriverGetMute()>>i->index & 1)
            snprintf(buffer,len,"%s (Mute)",buffer);
        break;
    default:
        if(item_can_be_edited(i))
            snprintf(buffer,len,"     %04x",item_get_value(i) & 0xffff);

        if(i->type == ITEM_SONGREQ)
        {
            int status = DriverGetSongStatus(i->index);
            int loopcnt = DriverGetLoopCount(i->index);

            switch(status&(SONG_STATUS_STARTING|SONG_STATUS_PLAYING))
            {
            case SONG_STATUS_NOT_PLAYING:
            default:
                break;
            case SONG_STATUS_PLAYING:
                songtime = DriverGetPlayingTime(i->index);
                if(status & SONG_STATUS_SUBSONG)
                    snprintf(buffer,len,"%s (Sub)",buffer);
                else if(loopcnt>0)
                    snprintf(buffer,len,"%s (Playing %2.0f:%02.0f) L%d",buffer,floor(songtime/60),floor(fmod(songtime,60)),loopcnt);
                else
                    snprintf(buffer,len,"%s (Playing %2.0f:%02.0f)",buffer,floor(songtime/60),floor(fmod(songtime,60)));
                if(status & SONG_STATUS_FADEOUT)
                    snprintf(buffer,len,"%s (Fading)",buffer);
                break;
            case SONG_STATUS_STARTING|SONG_STATUS_PLAYING:
            case SONG_STATUS_STARTING:
                snprintf(buffer,len,"%s (Starting)",buffer);
                break;
            }
        }
        break;
    }
    return buffer;
}

static void check_input()
{
    got_input=0;

    int increment=1;

    const uint8_t * kbd;
    kbd = SDL_GetKeyboardState(NULL);

    if(edit_mode)
    {
        switch(keycode)
        {
        case SDLK_DOWN:
        case SDLK_LEFT:
            increment=-increment;
        case SDLK_UP:
        case SDLK_RIGHT:
            increment *= kbd[SDL_SCANCODE_RSHIFT] ? 256 : 1;
            if(keycode == SDLK_UP || keycode == SDLK_DOWN)
                increment *= 16;
            edit_value = item_set_overflow(&item[select_pos],edit_value+increment);
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            item_set_value(&item[select_pos],edit_value);
        case SDLK_ESCAPE:
            edit_mode=0;
            break;
        }
    }
    else
    {
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
            edit_value=item_get_value(&item[select_pos]);
            if(item_can_be_edited(&item[select_pos]))
                edit_mode=1;
            break;
        case SDLK_s: // refresh
            switch(item[select_pos].type)
            {
            case ITEM_SONGREQ:
                Game->PlaylistControl = 0;
                DriverResetLoopCount();
                DriverStopSong(item[select_pos].index);
                break;
            case ITEM_VOICE:
                DriverSetSolo(DriverGetSolo() ^ 1<<item[select_pos].index);
                break;
            }

        default:
            break;
        }
    }

}

void scr_main2()
{
    if(refresh & R_SCR_MAIN2)
    {
        refresh &= ~R_SCR_MAIN2;
        generate_items();
        select_pos=0;
        blink=0;
        edit_mode=0;
    }
    set_color(1,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_50,COLOR_L_GREY);
    set_color(3,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_25,COLOR_L_GREY);
    set_color(5,1,FROWS-7,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
    set_color(49,0,1,FCOLUMNS,COLOR_D_BLUE,COLOR_L_GREY);
    SCRN(1,1,FCOLUMNS-2,"%s",DriverGetSongMessage());
    SCRN(3,1,FCOLUMNS-2,"Sound driver interface");

    if(item_cnt == 0)
    {
        SCRN(5,1,FCOLUMNS-2,"Nothing to do");
    }
    else
    {
        if(got_input)
            check_input();

        int y = 0;
        int i = select_pos;
        int offset = 0;
        int max = PLPAGE;
        if(item_cnt < max)
            max = item_cnt;
        if(i>(PLPAGE/2) && item_cnt > max)
            offset = i-(PLPAGE/2);
        if(offset+max > item_cnt)
            offset -= (offset+max)-item_cnt;

        for(y=0;y<max;y++)
        {
            i=y+offset;
            colorsel_t bg=COLOR_D_BLUE,fg=COLOR_L_GREY;

            if(select_pos == i)
            {
                bg = COLOR_N_BLUE;
                fg = COLOR_WHITE;
            }
            set_color(5+y,1,1,FCOLUMNS-2,bg,fg);
            SCRN(5+y,1,FCOLUMNS-2,"%-29s %s",item_display_name(&item[i],tempstring,29),item_display_value(&item[i],tempstring+80,40));
            if(select_pos == i && edit_mode)
            {
                SCRN(5+y,30,5,"%04x",edit_value);
                blink ^= 1;
                if(blink)
                    set_color(5+y,30,1,5,bg,COLOR_N_RED);
            }
        }

        if(edit_mode)
            SCRN(49,1,48,"ENTER: set value");
        else
            SCRN(49,1,48,"%s",item_can_be_edited(&item[select_pos])?"ENTER: edit mode":"");
    }

    return;

}
