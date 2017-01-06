#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "../qp.h"

#include "ui.h"

#define PLPAGE (FROWS-7)
    static int select_pos;
    static int pl_mode;
    static int kbd_transpose;
    static int kbd_flag;

static void select_pos_check()
{
    if(select_pos < 0)
        select_pos = 0;
    if(select_pos >= Game->SongCount)
        select_pos = Game->SongCount-1;
}

void scr_playlist_input2()
{
    got_input=0;

    switch(keycode)
    {
    case SDLK_l:
        pl_mode^=1;
        break;
    case SDLK_i:
        if(pl_mode==1)
            kbd_transpose+=24;
    case SDLK_d:
        if(pl_mode==1)
            kbd_transpose-=12;
        NOTICE("Keyboard display octave set to %d",-(kbd_transpose/12));
        break;
    case SDLK_8:
        if(pl_mode==1)
            kbd_flag ^= 1;
        NOTICE("Pitch mod display turned %s",kbd_flag&1?"ON":"OFF");
        break;
    case SDLK_9:
        if(pl_mode==1)
            kbd_flag ^= 2;
        NOTICE("Volume mod display turned %s",kbd_flag&2?"ON":"OFF");
        break;
    default:
        got_input=1;
    }

}

void scr_playlist_input()
{
    SDL_LockAudioDevice(Audio->dev);

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
        // force skip the boot song if RETURN is pressed twice
        // while boot song still playing
        if(DriverInterface->Type == DRIVER_QUATTRO && QDrv->BootSong && Game->PlaylistControl==2)
            QDrv->Track[0].SkipTrack=1;
        select_pos_check();
        Game->PlaylistPosition=select_pos;
    case SDLK_r:
        Game->PlaylistControl=2;
        break;
    case SDLK_f:
    case SDLK_s:
        Game->PlaylistControl = 0;
        int SongReq = Game->PlaylistSongID & 0x800 ? 8 : 0;
        if(keycode==SDLK_f)
            DriverFadeOutSong(SongReq);
        if(keycode==SDLK_s)
            DriverStopSong(SongReq);
        break;
    case SDLK_n:
        select_pos = Game->PlaylistPosition+1;
        select_pos_check();
        Game->PlaylistPosition=select_pos;
        Game->PlaylistControl=2;
        break;
    case SDLK_b:
        select_pos = Game->PlaylistPosition-1;
        select_pos_check();
        Game->PlaylistPosition=select_pos;
        Game->PlaylistControl=2;
        break;
    default:
        got_input=1;
    }

    SDL_UnlockAudioDevice(Audio->dev);
}

#define MAX_VOICES 32
void scr_playlist_kbd2()
{
    int i;
    int id=0, id2=0;
    int16_t pitch;
    int has_drums=0;

    int cnt = DriverGetVoiceCount();
    if(cnt>MAX_VOICES) cnt=MAX_VOICES;

    struct QP_DriverVoiceInfo *vi = calloc(MAX_VOICES,sizeof(*vi)), *V;
    uint8_t pcm_wave[256] = {0};
    uint16_t pcm_voice[8] = {0};

    // Get voice info
    for(i=0;i<cnt;i++)
    {
        if(!DriverGetVoiceInfo(i,&vi[id]))
        {
            if((vi[id].VoiceType&0x0f) == VOICE_TYPE_PERCUSSION)
            {
                has_drums=1;
                if(vi[id].Status == VOICE_STATUS_PLAYING)
                {
                    pcm_wave[vi[id].Preset&0xff] = 1;
                    pcm_voice[id2&7]=(vi[id].Preset&0xff)|0x8000;
                }
                id2++;
            }
            else
                id++;
        }
    }
    V = vi;
    cnt=id;

    int bg,color;
    set_color(5,39,40,2,COLOR_BLACK,COLOR_BLACK);
    bg = COLOR_D_BLUE|CFLAG_KEYBOARD;

    id=0;
    int key,center,vol,pan;
    int x,y,flag;
    char pandir;
    for(i=0;i<MAX_VOICES;i++)
    {
        key=0;
        // Calculate position
        x = (i&16) ? 40 : 0;
        y = ((i&15)>>1)*5 + ((i&1) * 2);
        // Shift odd rows
        flag = (i&1) ? CFLAG_YSHIFT_50 : 0;

        // Display PCM sample id matrix
        if(has_drums && i>=16 && i<24)
        {
            color = COLOR_L_GREY;
            set_color(5+y,1+x,2,32,bg|flag,color|CFLAG_KEYBOARD);
            set_color(5+y,33+x,2,6,bg|flag,COLOR_L_GREY|CFLAG_KEYBOARD);

            ui_array(5+y,1+x,32,&pcm_wave[(i&7)<<5]);

            key = i-16;
            if(pcm_voice[key&7] & 0x8000)
            {
                set_color(5+y,36+x,1,5,bg|flag,COLOR_WHITE|CFLAG_KEYBOARD);
                SCRN(5+y,35+x,16,"w%03d",pcm_voice[key&7]&0x7fff);
            }
            continue;
        }

        // Display keyboard status
        color = (V->Status&VOICE_STATUS_ACTIVE) ? COLOR_WHITE : COLOR_N_GREY;
        set_color(5+y,1+x,2,28,bg|flag,color|CFLAG_KEYBOARD);
        set_color(5+y,29+x,2,10,bg|flag,COLOR_L_GREY|CFLAG_KEYBOARD);

        if(V->Status & VOICE_STATUS_ACTIVE)
        {
            set_color(5+y,30+x,1,2,bg|flag,color|CFLAG_KEYBOARD);
            set_color(5+y,33+x,1,6,bg|flag,color|CFLAG_KEYBOARD);
            set_color(6+y,30+x,1,4,bg|flag,color|CFLAG_KEYBOARD);
            set_color(6+y,36+x,1,3,bg|flag,color|CFLAG_KEYBOARD);
        }

        pitch=V->Key;
        if(kbd_flag&1)
            pitch = (V->Pitch)>>8;

        if(V->Status&VOICE_STATUS_PLAYING)
            key=kbd_transpose+pitch;

        ui_keyboard(5+y,1+x,8,key);

        center=0;
        // display pan
        switch(V->PanType&0x0f)
        {
        default:
        case PAN_TYPE_NONE:
            break;
        case PAN_TYPE_UNSIGNED:
            center=128;
        case PAN_TYPE_SIGNED:
            pandir = 'L';
            if(V->Pan == center)
                pandir = 'C';
            else if((~V->PanType & PAN_TYPE_INVERT && V->Pan > center) || (V->PanType & PAN_TYPE_INVERT && V->Pan < center))
                pandir = 'R';

            pan = abs(V->Pan);
            if(V->PanType==PAN_TYPE_SIGNED)
            {
                SCRN(5+y,35+x,16,"%c%s%02d",
                    pandir,
                    (pan>99) ? "" : "+",
                    pan);
            }
            else
            {
                SCRN(5+y,35+x,16,"%c%03d",
                    pandir,
                    pan);
            }
            break;
        }

        vol = (kbd_flag&2) ? V->VolumeMod : V->Volume;

        SCRN(5+y,29+x,16,"t%02xc%1x",
             V->Track,
             V->Channel);
        SCRN(6+y,29+x,16,"w%04d v%03d",
             V->Preset&0x1fff,
             (vol>255) ? 255 : vol);

        V++;
    }
    free(vi);
}

void scr_playlist_kbd()
{
    // only supported for quattro atm
    if(DriverInterface->Type == DRIVER_SYSTEM2)
        return scr_playlist_kbd2();
    else if(DriverInterface->Type != DRIVER_QUATTRO)
        return;

    Q_Voice* V;
    char pandir;
    int8_t pan;
    int16_t vol;
    int16_t pitch;
    int16_t temp;
    set_color(5,39,40,2,COLOR_BLACK,COLOR_BLACK);
    int i,n;
    int x,y,flag,bg,color;
    bg = COLOR_D_BLUE|CFLAG_KEYBOARD;
    for(i=0;i<32;i++)
    {
        V = &QDrv->Voice[i];
        n=0;
        x = (i&16) ? 40 : 0;
        y = ((i&15)>>1)*5 + ((i&1) * 2);
        flag = (i&1) ? CFLAG_YSHIFT_50 : 0;
        color = V->TrackNo ? COLOR_WHITE : COLOR_N_GREY;
        set_color(5+y,1+x,2,28,bg|flag,color|CFLAG_KEYBOARD);
        set_color(5+y,29+x,2,10,bg|flag,COLOR_L_GREY|CFLAG_KEYBOARD);

        if(V->TrackNo)
        {
            set_color(5+y,30+x,1,2,bg|flag,color|CFLAG_KEYBOARD);
            set_color(5+y,33+x,1,6,bg|flag,color|CFLAG_KEYBOARD);
            set_color(6+y,30+x,1,4,bg|flag,color|CFLAG_KEYBOARD);
            set_color(6+y,36+x,1,3,bg|flag,color|CFLAG_KEYBOARD);
        }

        pitch=V->BaseNote;
        if(kbd_flag&1)
            pitch = ((V->Pitch+V->PitchEnvMod+V->LfoMod)&0xff00)>>8;

        if(V->Enabled==1)
            n=kbd_transpose-2+pitch;

        ui_keyboard(5+y,1+x,8,n);

        temp=0;
        switch(V->PanMode)
        {
        case Q_PANMODE_IMM:
        default:
            temp = (signed)V->Pan;
            break;
        case Q_PANMODE_ENV:
        case Q_PANMODE_ENV_SET:
            temp = (V->PanEnvTarget-V->PanEnvValue)>>8;
            break;
        case Q_PANMODE_REG:
        case Q_PANMODE_POSREG:
            if(V->PanSource)
                temp = *V->PanSource & 0xff;
            break;
        }
        pan = temp;
        if(!pan)
            pandir='C';
        else if(pan<0)
            pandir='L';
        else
            pandir='R';
        pan = abs(pan);

        vol = V->Volume;
        if(kbd_flag&2 && V->TrackVol)
            vol += *V->TrackVol;

        SCRN(5+y,29+x,16,"t%02xc%1x %c%s%02d",
             V->TrackNo ? V->TrackNo-1 : 0,
             V->ChannelNo,
             pandir,
             (pan>99) ? "" : "+",
             pan);
        SCRN(6+y,29+x,16,"w%04d v%03d",
             V->WaveNo&0x1fff,
             (vol>255) ? 255 : vol);
    }
}


void scr_playlist_list(int ypos,int height)
{
    int y = 0;
    int i = select_pos;
    int offset = 0;
    int max = height;
    if(Game->SongCount < max)
        max = Game->SongCount;
    if(i>(height/2) && Game->SongCount > max)
        offset = i-(height/2);
    if(offset+max > Game->SongCount)
        offset -= (offset+max)-Game->SongCount;

    for(y=0;y<max;y++)
    {
        i=y+offset;
        colorsel_t bg=COLOR_D_BLUE,fg=COLOR_L_GREY;

        if(select_pos == i)
            bg = COLOR_N_BLUE;
        if(Game->PlaylistPosition == i)
            fg = COLOR_WHITE;

        set_color(ypos+y,1,1,FCOLUMNS-2,bg,fg);

        SCRN(ypos+y,1,FCOLUMNS-2,"%02d %s",i+1,Game->Playlist[i].Title);
    }
}



void scr_playlist()
{
    static int blink;
    int disp_timer;

    if(refresh & R_SCR_PLAYLIST)
    {
        refresh &= ~R_SCR_PLAYLIST;
        select_pos=0;
        pl_mode=0;
        kbd_transpose=0;
        kbd_flag=0;
    }

    set_color(1,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_50,COLOR_L_GREY);
    set_color(3,1,1,FCOLUMNS-2,COLOR_D_BLUE|CFLAG_YSHIFT_25,COLOR_L_GREY);
    set_color(5,1,FROWS-7,FCOLUMNS-2,COLOR_D_BLUE,COLOR_L_GREY);
    set_color(49,0,1,FCOLUMNS,COLOR_D_BLUE,COLOR_L_GREY);
    SCRN(1,1,FCOLUMNS-2,"%s",DriverGetSongMessage());

    int SongReq = Game->PlaylistSongID & 0x800 ? 8 : 0;

    disp_timer=0;
    switch(Game->PlaylistControl)
    {
    case 0:
        SCRN(3,1,FCOLUMNS-2,"Playlist not currently active");
    default:
        break;
    case 1:
        disp_timer=1;
    case 2:
        SCRN(3,1,FCOLUMNS-6,"Playing song %02d: %s",
             Game->PlaylistPosition+1,
             Game->Playlist[Game->PlaylistPosition].Title);

        if(Audio->Enabled && disp_timer)
        {
            blink++;
            if(blink&0x20)
                disp_timer=0;
        }

        if(disp_timer)
        {
            double songtime = DriverGetPlayingTime(SongReq);
            SCRN(3,FCOLUMNS-6,6,"%2.0f:%02.0f",
                floor(songtime/60),floor(fmod(songtime,60)));
        }
        break;
    }

    if(Game->SongCount)
    {
        SCRN(49,1,48,"ENTER: play song, S: stop, F: fade, P: pause");
        if(got_input)
            scr_playlist_input();
    }
    if(got_input)
        scr_playlist_input2();

    int ypos=5;
    int max=PLPAGE;
    if(pl_mode==1)
    {
        scr_playlist_kbd();
        ypos+=40;
        max-=40;
    }

    if(Game->SongCount == 0)
    {
        SCRN(ypos,1,FCOLUMNS-2,"Playlist not defined.");
    }
    else
    {
        scr_playlist_list(ypos,max);
    }

}
