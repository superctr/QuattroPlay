#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "../qp.h"
#include "ui.h"
#include "scr_main.h"
#include "system2.h"
#include "quattro.h" /* need it for note table */

void ui_info_s2_track(int id,int ypos)
{
    int i, j, x, y;
    uint8_t note, oct;

    S2X_State *S = DriverInterface->Driver;
    S2X_Track *T = &S->Track[id];

    set_color(ypos,44,6,35,COLOR_D_BLUE,COLOR_L_GREY);
    // todo: print subs...

    if(S->ParentSong[id] == id)
    {
        j=0;
        for(i=0;i<S2X_MAX_TRACKS;i++)
        {
            if(i != id && S->ParentSong[i] == id)
            {
                if(j==0)
                    x = SCRN(ypos,44,20,"Subs: ");

                j += SCRN(ypos,44+x+j,6,"%c%02x",j==0 ? ' ' : ',', i);
            }
        }
    }
    else if(S->ParentSong[id] != S2X_MAX_TRACKS)
    {
        SCRN(ypos,44,40,"Sub of %02x",S->ParentSong[id]);
    }
    ypos++;

    double bpm = 0;
    if( (T->BaseTempo*T->Tempo) > 0 )
        bpm = (double)1800/ (T->BaseTempo*T->Tempo);
    if(S->DriverType==S2X_TYPE_SYSTEM1)
        bpm/=2;

    //                        .............
    SCRN(ypos,44,40,"Pos:   %06x  BPM:%7.2f  Vol:%3d",T->Position+T->PositionBase,bpm,T->TrackVolume);
    ypos++;

    if(T->SubStackPos)
    {
        //                            ........
        j = SCRN(ypos,44,20,"Stack: %06x",T->SubStack[T->SubStackPos-1]+T->PositionBase);
        if(T->SubStackPos > 1)
            j += SCRN(ypos,44+j,20,",   %06x",T->SubStack[T->SubStackPos-2]+T->PositionBase);
        if(T->SubStackPos > 2)
            j += SCRN(ypos,44+j,5," +%2d",T->SubStackPos-2);
    }
    ypos++;

    if(T->RepeatStackPos)
    {
        //                            ........
        j = SCRN(ypos,44,20,"Repeat:%2d %06x",T->RepeatCount[T->RepeatStackPos-1],T->RepeatStack[T->RepeatStackPos-1]+T->PositionBase);
        if(T->RepeatStackPos > 1)
            j += SCRN(ypos,44+j,20,",%2d %06x",T->RepeatCount[T->RepeatStackPos-2],T->RepeatStack[T->RepeatStackPos-2]+T->PositionBase);
        if(T->RepeatStackPos > 2)
            j += SCRN(ypos,44+j,5," +%2d",T->RepeatStackPos-2);
    }
    ypos++;

    if(T->LoopStackPos)
    {
        //                            ........
        j = SCRN(ypos,44,20,"Loop:  %2d %06x",T->LoopCount[T->LoopStackPos-1],T->LoopStack[T->LoopStackPos-1]+T->PositionBase);
        if(T->LoopStackPos > 1)
            j += SCRN(ypos,44+j,20,",%2d %06x",T->LoopCount[T->LoopStackPos-2],T->LoopStack[T->LoopStackPos-2]+T->PositionBase);
        if(T->LoopStackPos > 2)
            j += SCRN(ypos,44+j,5," +%2d",T->LoopStackPos-2);
    }
    ypos+=2;

    int type,index,val;

    colorsel_t c1, c2;
    switch(displaysection%2)
    {
    case 0:
        //ui_pattern_disp(id);
        QP_PatternGenerate(id,&pattern);

        if(!pattern.len)
            break;

        ypos++;

        SCRN(ypos,44,4,"No.");
        for(i=0;i<S2X_MAX_TRKCHN;i++)
        {
            c1 = COLOR_BLACK;
            c2 = COLOR_WHITE;
            x = 48+(i*4);
            if(T->Channel[i].Enabled)
            {
                c1 = COLOR_D_BLUE;
                SCRN(ypos,x+1,4,"%02x",T->Channel[i].VoiceNo);
            }
            set_color(ypos,x,1,3,c1,c2);
        }
        ypos++;

        // wave/volume
        // display modes to be added later:
        // envelope/pan
        // lfo/pitch env
        // detune/porta
        // delay/length

        SCRN(ypos,44,4,"Wav");
        for(i=0;i<S2X_MAX_TRKCHN;i++)
        {
            val=T->Channel[i].Vars[S2X_CHN_WAV];
            if(T->Channel[i].Enabled)
            {
                type = S2X_GetVoiceType(S,T->Channel[i].VoiceNo);
                index = S2X_GetVoiceIndex(S,T->Channel[i].VoiceNo,type);
                if(type==S2X_VOICE_TYPE_PCM && S->PCM[index].Flag&0x80 )
                    val= S->PCM[index].WaveNo | 0x8000;
                else if(type==S2X_VOICE_TYPE_FM && S->FM[index].Flag&0x80)
                    val= S->FM[index].InsNo | 0x8000;
            }

            x = 48+(i*4);
            // hex/dec display toggle?
            SCRN(ypos,x,4,"%03x",val&0xff);

            c1 = COLOR_BLACK;
            if(!T->Channel[i].Enabled)
                c2 = COLOR_D_GREY;
            else if(val&0x8000)
                c2 = COLOR_L_GREY;
            else
                c2 = COLOR_N_GREY;

            set_color(ypos,x,2,3,c1,c2);
        }
        ypos++;

        SCRN(ypos,44,4,"Vol");
        for(i=0;i<S2X_MAX_TRKCHN;i++)
        {
            x = 48+(i*4);
            SCRN(ypos,x,4,"%03d",T->Channel[i].Vars[S2X_CHN_VOL]);
        }
        ypos++;

        SCRN(ypos,44,4,"Frq");
        for(i=0;i<S2X_MAX_TRKCHN;i++)
        {
            c1 = COLOR_BLACK;
            c2 = COLOR_WHITE;
            x = 48+(i*4);
            note = T->Channel[i].Vars[S2X_CHN_FRQ];
            y = T->Channel[i].Vars[S2X_CHN_VOF];
            if(T->Channel[i].Enabled || (!note && y))
            {
                if(!note && y)
                {
                    SCRN(ypos,x,4,"%03d",y-1);
                    // base voice number
                    oct = (S->DriverType==S2X_TYPE_NA) ? 8 : 16;
                    // grey out if the voice is not currently playing
                    oct = (T->Channel[i].Enabled) ? T->Channel[i].VoiceNo : oct+i;
                    if(!S->SE[i].Type || S->SE[i].Track != id || (DriverGetVoiceStatus(oct)&0xf000) != 0xf000)
                        c2 = COLOR_L_GREY;
                }
                else if(!note)
                    SCRN(ypos,x,4,"---");
                else if(note == 0xff)
                    SCRN(ypos,x,4,"===");
                else
                {
                    if(T->Channel[i].VoiceNo > 23)
                        note += 4; // for FM
                    note += T->Channel[i].Vars[S2X_CHN_TRS];
                    oct = (note-3)/12;
                    note %= 12;
                    SCRN(ypos,x,4,"%s%d",Q_NoteNames[note],oct);
                }
                c1 = COLOR_D_BLUE;
            }
            set_color(ypos,x,1,3,c1,c2);
        }
        ypos++;

        for(j=0;j<pattern.len;j++)
        {
            SCRN(ypos,44,4,"+%2d",j);
            for(i=0;i<Q_MAX_TRKCHN;i++)
            {
                x = 48+(i*4);

                c1 = T->Channel[i].Enabled ? COLOR_D_BLUE : COLOR_BLACK;
                y = pattern.pat[j][i];

                if(y == -1)
                {
                    c2 = COLOR_D_GREY;
                    SCRN(ypos,x,4,"---");
                }
                else
                {
                    c2 = COLOR_L_GREY;

                    if((y&0xff00) == 0x100)
                    {
                        SCRN(ypos,x,4,"===");
                    }
                    else if((y&0xff00) == 0)
                    {
                        note = y&0xff;
                        if(T->Channel[i].VoiceNo > 23)
                            note += 4; // for FM
                        oct = (note-3)/12;
                        note %= 12;
                        SCRN(ypos,x,4,"%s%d",Q_NoteNames[note],oct);
                    }
                    else
                    {
                        c1 = COLOR_D_BLUE;
                        SCRN(ypos,x,4,"%3d",y&0xff);
                    }
                }
                set_color(ypos,x,1,3,c1,c2);
            }
            ypos++;
        }
        break;
    case 1:
        SCRN(ypos,44,4,"this page not implemented yet");
        break;
    }
}
