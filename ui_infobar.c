/*
    User interface code for QuattroPlay

    Sorry for the unreadability of this code, it's very much a quick and dirty job...
*/

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "math.h"

#include "ui.h"
#include "ui_menu.h"
#include "drv/quattro.h"
#include "drv/tables.h"


void ui_info_track(int id,int ypos)
{
    int i, j, x, y;
    uint8_t note, oct;

    Q_Track * T = &QDrv->Track[id];

    set_color(ypos,44,6,35,COLOR_D_BLUE,COLOR_L_GREY);
    // todo: print subs...

    if(QDrv->ParentSong[id] == id)
    {
        j=0;
        for(i=0;i<QDrv->TrackCount;i++)
        {
            if(i != id && QDrv->ParentSong[i] == id)
            {
                if(j==0)
                    x = snprintf(&text[ypos][44],20,"Subs: ");

                j += snprintf(&text[ypos][44+x+j],6,"%c%02x",j==0 ? ' ' : ',', i);
            }
        }
    }
    else if(QDrv->ParentSong[id] != Q_MAX_TRACKS)
    {
        snprintf(&text[ypos][44],40,"Sub of %02x",QDrv->ParentSong[id]);
    }
    ypos++;

    double bpm = 0;

    if( (T->BaseTempo*T->Tempo) > 0 )
        bpm = (double)115200/ (T->BaseTempo*T->Tempo);

    //                        .............
    snprintf(&text[ypos][44],40,"Pos:   %06x  BPM:%7.2f  Vol:%3d",T->Position,bpm,T->GlobalVolume);
    ypos++;

    if(T->SubStackPos)
    {
        //                            ........
        j = snprintf(&text[ypos][44],20,"Stack: %06x",T->SubStack[T->SubStackPos-1]);
        if(T->SubStackPos > 1)
            j += snprintf(&text[ypos][44+j],20,",   %06x",T->SubStack[T->SubStackPos-2]);
        if(T->SubStackPos > 2)
            j += snprintf(&text[ypos][44+j],3," +%2d",T->SubStackPos-2);
    }
    ypos++;

    if(T->RepeatStackPos)
    {
        //                            ........
        j = snprintf(&text[ypos][44],20,"Repeat:%2d %06x",T->RepeatCount[T->RepeatStackPos-1],T->RepeatStack[T->RepeatStackPos-1]);
        if(T->RepeatStackPos > 1)
            j += snprintf(&text[ypos][44+j],20,",%2d %06x",T->RepeatCount[T->RepeatStackPos-2],T->RepeatStack[T->RepeatStackPos-2]);
        if(T->RepeatStackPos > 2)
            j += snprintf(&text[7][44+j],3," +%2d",T->RepeatStackPos-2);
    }
    ypos++;

    if(T->LoopStackPos)
    {
        //                            ........
        j = snprintf(&text[ypos][44],20,"Loop:  %2d %06x",T->LoopCount[T->LoopStackPos-1],T->LoopStack[T->LoopStackPos-1]);
        if(T->LoopStackPos > 1)
            j += snprintf(&text[ypos][44+j],20,",%2d %06x",T->LoopCount[T->LoopStackPos-2],T->LoopStack[T->LoopStackPos-2]);
        if(T->LoopStackPos > 2)
            j += snprintf(&text[ypos][44+j],3," +%2d",T->LoopStackPos-2);
    }
    ypos+=2;

    colorsel_t c1, c2;
    switch(displaysection%2)
    {
    case 0:
        ui_pattern_disp(id);

        if(!trackpattern_length)
            break;

        ypos++;

        snprintf(&text[ypos][44],4,"No.");
        for(i=0;i<Q_MAX_TRKCHN;i++)
        {
            c1 = COLOR_BLACK;
            c2 = COLOR_WHITE;
            x = 48+(i*4);
            if(T->Channel[i].Enabled)
            {
                c1 = COLOR_D_BLUE;
                snprintf(&text[ypos][x+1],4,"%02x",T->Channel[i].VoiceNo);
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

        snprintf(&text[ypos][44],4,"Wav");
        for(i=0;i<Q_MAX_TRKCHN;i++)
        {
            x = 48+(i*4);
            // hex/dec display toggle?
            snprintf(&text[ypos][x],4,"%03x",T->Channel[i].Enabled && T->Channel[i].Voice->Enabled ?
                     T->Channel[i].Voice->WaveNo & 0xfff : T->Channel[i].WaveNo & 0xfff);

            c1 = COLOR_BLACK;
            if(!T->Channel[i].Enabled)
                c2 = COLOR_D_GREY;
            else if(T->Channel[i].Voice->Enabled)
                c2 = COLOR_L_GREY;
            else
                c2 = COLOR_N_GREY;

            set_color(ypos,x,2,3,c1,c2);
        }
        ypos++;

        snprintf(&text[ypos][44],4,"Vol");
        for(i=0;i<Q_MAX_TRKCHN;i++)
        {
            x = 48+(i*4);
            snprintf(&text[ypos][x],4,"%03d",T->Channel[i].Enabled && T->Channel[i].Voice->Enabled ?
                     T->Channel[i].Voice->Volume : T->Channel[i].Volume);
        }
        ypos++;

        snprintf(&text[ypos][44],4,"Frq");
        for(i=0;i<Q_MAX_TRKCHN;i++)
        {
            c1 = COLOR_BLACK;
            c2 = COLOR_WHITE;
            x = 48+(i*4);
            if(T->Channel[i].Enabled)
            {
                y = T->Channel[i].ChannelLink ? (T->Channel[i].ChannelLink-1)&7 : i;

                note = T->Channel[y].KeyOnNote;
                j = T->Channel[y].KeyOnType & 0x7f;

                if(~T->Channel[y].KeyOnType & 0x80)
                    snprintf(&text[ypos][x],4,"---");
                else if(j == 0 && note == 0x7f)
                    snprintf(&text[ypos][x],4,"===");
                else
                {
                    if(j!=0 || note > 0x7f)
                        note = T->Channel[i].BaseNote;
                    note += T->Channel[i].Transpose;
                    oct = (note-3)/12;
                    note %= 12;
                    snprintf(&text[ypos][x],4,"%s%d",Q_NoteNames[note],oct);
                }

                //else if(j == 0 && T->Channel[i].KeyOnNote > 0x7f)
                //    snprintf(&text[ypos][x],4,"w%02x",T->Channel[i].KeyOnNote&0x7f);
                //else if(j == 0)
                //    snprintf(&text[ypos][x],4,"%s%d",Q_NoteNames[y],oct);
                //else
                //    snprintf(&text[ypos][x],4,"%3d",T->Channel[i].KeyOnNote);
                c1 = COLOR_D_BLUE;
            }
            set_color(ypos,x,1,3,c1,c2);
        }
        ypos++;

        for(j=0;j<trackpattern_length;j++)
        {
            snprintf(&text[ypos][44],4,"+%2d",j);
            for(i=0;i<Q_MAX_TRKCHN;i++)
            {
                x = 48+(i*4);

                c1 = T->Channel[i].Enabled ? COLOR_D_BLUE : COLOR_BLACK;
                y = trackpattern[j][i];

                if(y == -1)
                {
                    c2 = COLOR_D_GREY;
                    snprintf(&text[ypos][x],4,"---");
                }
                else
                {
                    c2 = COLOR_L_GREY;

                    if((y&0x0100) == 0x100)
                    {
                        snprintf(&text[ypos][x],4,"r%02x",y&0xff);
                    }
                    else if(((y&0xfe00) ==0x000) && ((y&0xff) == 0x7f))
                    {
                        snprintf(&text[ypos][x],4,"===");
                    }
                    else if(((y&0xfe00) ==0x000) && ((y&0xff) > 0x7f))
                    {
                        snprintf(&text[ypos][x],4,"w%02x",y&0x7f);
                    }
                    else if((y&0xfe00) ==0x000)
                    {
                        note = y&0xff;
                        oct = (note-3)/12;
                        note %= 12;
                        snprintf(&text[ypos][x],4,"%s%d",Q_NoteNames[note],oct);
                    }
                    else
                    {
                        snprintf(&text[ypos][x],4,"%3d",y&0xff);
                    }
                }
                set_color(ypos,x,1,3,c1,c2);
            }
            ypos++;
        }
        break;
    case 1:
        snprintf(&text[ypos][44],4,"Trk");

        // Display bytes from track struct
        for(i=0;i<0x40;i++)
        {
            y = ypos+(i/16);
            x = 48+((i%16)*2);

            c1 = ((i/16)%2)==0 ? COLOR_BLACK : COLOR_D_BLUE;
            c2 = (i%2)==0 ? COLOR_WHITE : COLOR_L_GREY;
            snprintf(&text[y][x],3,"%02x",Q_ReadTrackInfo(QDrv,id,i));
            set_color(y,x,1,2,c1,c2);
        }
        ypos+=2;
        // Display bytes from track channel struct.
        for(j=0;j<8;j++)
        {
            snprintf(&text[ypos+(j*2)][44],4,"Ch%d",j);
            for(i=0;i<0x20;i++)
            {
                y = ypos+(j*2)+(i/16);
                x = 48+((i%16)*2);

                c1 = ((i/16)%2)==0 ? COLOR_BLACK : COLOR_D_BLUE;
                c2 = (i%2)==0 ? COLOR_WHITE : COLOR_L_GREY;
                snprintf(&text[y][x],3,"%02x",Q_ReadChannelInfo(QDrv,id,j,i));
                set_color(y,x,1,2,c1,c2);
            }
        }
        ypos+=j*2;

#ifdef DEBUG
#ifndef Q_DISABLE_LOOP_DETECTION
        y = QDrv->SongRequest[id]&0x7ff;
        snprintf(&text[ypos][44],40,"Sub loop id: %08x count: %3d",QDrv->TrackLoopId[y],QDrv->TrackLoopCount[y]);
#endif // Q_DISABLE_LOOP_DETECTION
#endif // DEBUG
        break;
    }
}

const char* EnvelopeState_KOn[Q_ENV_STATE_MAX] =
{
    "Disabled","Attack","Decay","Sustain"
};
const char* EnvelopeState_KOff[Q_ENV_STATE_MAX] =
{
    "Off","Attack","Release","Sustain"
};

void ui_info_voice(int id,int ypos)
{
    int tempypos;

    Q_Voice* V = &QDrv->Voice[id];

    set_color(ypos,44,28,40,COLOR_D_BLUE,COLOR_L_GREY);

    snprintf(&text[ypos++][44],40,"Chip registers:");
    snprintf(&text[ypos++][44],40,"%6s:  %04x%6s:%04x%6s:%04x",
             "Flag",    QDrv->Chip.v[id].flags,
             "VolF",    QDrv->Chip.v[id].vol_f,
             "VolR",    QDrv->Chip.v[id].vol_r);
    snprintf(&text[ypos++][44],40,"%6s:%06x%6s:%04x (%5.0f Hz)",
             "Pos",     QDrv->Chip.v[id].pos & 0xffffff,
             "Freq",    QDrv->Chip.v[id].freq,
             ((double)QDrv->Chip.v[id].freq/0x10000)*QDrv->Chip.rate);
    snprintf(&text[ypos++][44],40,"%6s:%02x%04x%6s:%04x%6s:%04x",
             "Start",   QDrv->Chip.v[id].wave_bank&0xff,QDrv->Chip.v[id].wave_start,
             "End",     QDrv->Chip.v[id].wave_end,
             "Loop",    QDrv->Chip.v[id].wave_loop);
    ypos++;

    if(V->GateTimeLeft)
        snprintf(&text[ypos][60],40,"(Time Left: %3d/%3d)",V->GateTimeLeft,V->GateTime);

    snprintf(&text[ypos++][44],40,"Voice %s",V->Enabled ? "Enabled":"Disabled");

    tempypos=ypos;


    snprintf(&text[ypos++][45],40,"%-10s%04x (Pos %06x)",
             "WaveNo",      V->WaveNo,V->WavePos);
    snprintf(&text[ypos++][45],40,"%-10s%04x (%04x %s)",
             "Envelope",    V->EnvNo,V->EnvValue,
             (V->Enabled == 1) ? EnvelopeState_KOn[V->EnvState] : EnvelopeState_KOff[V->EnvState]);
    snprintf(&text[ypos++][45],40,"%-10s%4d +%4d",
             "Volume",      V->Volume,
             V->TrackVol ? (*V->TrackVol)&0xff : 0);
    if(V->PanMode == Q_PANMODE_IMM)
        snprintf(&text[ypos][45],40,"%-10s%4d",
                 "Pan",     (int8_t)V->Pan);
    else if(V->PanMode == Q_PANMODE_ENV || V->PanMode == Q_PANMODE_ENV_SET)
        snprintf(&text[ypos][45],40,"%-10s  %02x = %04x (%04x)",
                 "PanEnv",  V->Pan,(uint16_t)(V->PanEnvTarget-V->PanEnvValue),V->PanEnvDelta);
    else if(V->PanMode == Q_PANMODE_POSENV || V->PanMode == Q_PANMODE_POSENV_SET)
        snprintf(&text[ypos][45],40,"%-10s  %02x = ????",
                 "PosEnv",  V->Pan);
    else if(V->PanMode == Q_PANMODE_REG)
        snprintf(&text[ypos][45],40,"%-10s  %02x = %04x",
                 "PanReg",  V->Pan,
                 V->PanSource ? *V->PanSource : 0);
    else if(V->PanMode == Q_PANMODE_POSREG)
        snprintf(&text[ypos][45],40,"%-10s  %02x = %4d,%4d",
                 "PosReg",  V->Pan,
                 V->PanSource ? (int8_t)(*V->PanSource>>8) : 0,
                 V->PanSource ? (int8_t)(*V->PanSource&0xff) : 0);
    else
        snprintf(&text[ypos][45],40,"%-10s%4d,%4d",
                 "Pos",     (int8_t)V->Pan,(int8_t)V->PanMode);
    if(V->PanMode == Q_PANMODE_POSENV_SET || V->PanMode == Q_PANMODE_ENV_SET)
        snprintf(&text[ypos][51],4,"Set");
    ypos++;

    uint8_t note, oct;

    note = V->BaseNote-V->Transpose;
    oct = (note-3)/12;
    note %= 12;

    if(V->PitchReg)
    {
        snprintf(&text[ypos++][45],40,"%-10s  %02x (%02x)",
                 "Pitch Reg.",V->PitchReg,QDrv->Register[V->PitchReg]);
        ypos+=1;
    }
    else
    {
        snprintf(&text[ypos++][45],40,"%-10s %s%d (%+4d = %s%d)",
                 "Note",    Q_NoteNames[note],  oct,
                 (int8_t)V->Transpose, Q_NoteNames[V->BaseNote%12], (V->BaseNote-3)/12);
        snprintf(&text[ypos++][45],40,"%-10s%4d",
                 "Detune",    V->Detune);
    }

    snprintf(&text[ypos++][45],40,"%-10s%04x",
             "PitchEnv",V->PitchEnvNo);
    snprintf(&text[ypos++][45],40,"%-10s%4d",
             "LFO",       V->LfoNo);
    snprintf(&text[ypos++][45],40,"%-10s%04x",
             "Offset",    V->SampleOffset);

    if(!V->Enabled)
    {
        set_color(tempypos,44,ypos-tempypos,40,COLOR_D_BLUE,COLOR_D_GREY);
    }

    ypos+=2;

    uint8_t wavepos, base, step, val;

    switch(displaysection%2)
    {
    case 0:
        note = V->BaseNote-V->Transpose;
        oct = (note-3)/12;
        note %= 12;

        snprintf(&text[ypos++][44],40,"Frequency");

        snprintf(&text[ypos++][45],40,"%-16s%04x + %04x = %04x",
                 "Wave Rate",V->WaveTranspose,QDrv->BasePitch,(uint16_t)(V->WaveTranspose+QDrv->BasePitch));
        snprintf(&text[ypos++][45],40,"%-28s= %04x",
                 "Pitch Target",V->PitchTarget);
        snprintf(&text[ypos++][45],40,"%-23s%4d = %04x",
                 "Portamento",V->Portamento,V->Pitch);
        snprintf(&text[ypos++][45],40,"%-23s%04x = %04x",
                 "Pitch Envelope",V->PitchEnvNo,V->PitchEnvMod);
        snprintf(&text[ypos++][45],40,"%-25s%02x = %04x",
                 "LFO Preset",V->LfoNo,V->LfoMod);
        snprintf(&text[ypos++][45],40,"%-28s= %04x",
                 "Final Pitch",(uint16_t)(QDrv->BasePitch+V->WaveTranspose+V->Pitch+V->PitchEnvMod+V->LfoMod));
        snprintf(&text[ypos++][45],40,"%-28s= %04x",
                 "Frequency Register",V->FreqReg);
        break;
    case 1:
        snprintf(&text[ypos++][44],40,"LFO %s",V->LfoEnable ? "Enabled":"Disabled");

        snprintf(&text[ypos++][45],40,"%-8s%02x  %-4s%06x",
                 "No",V->LfoNo,
                 "Pos",QDrv->TableOffset[Q_TOFFSET_LFOTABLE]+(8*(V->LfoNo-1)));
        snprintf(&text[ypos++][45],40,"%-8s%02x  %-8s%02x, %-6s%04x",
                 "Delay",V->LfoDelay,
                 "Wave",V->LfoWaveform,
                 "Phase",V->LfoPhase);
        snprintf(&text[ypos++][45],40,"%-6s%04x  %-6s%04x  %-6s%04x",
                 "Freq",V->LfoFreq,
                 "=>",V->LfoFreqTarget,
                 "Delta",V->LfoFreqDelta);
        snprintf(&text[ypos++][45],40,"%-6s%04x  %-6s%04x  %-6s%04x",
                 "Depth",V->LfoDepth,
                 "=>",V->LfoDepthTarget,
                 "Delta",V->LfoDepthDelta);

        wavepos = ((V->LfoWaveform&0x0f)<<4)|((V->LfoPhase&0x1e0)>>5);
        base = (Q_LfoWaveTable[wavepos]&0xff00)>>8; // base
        step = ((Q_LfoWaveTable[wavepos]&0x007f) * (V->LfoPhase&0x1f))>>4;

        if(Q_LfoWaveTable[wavepos]&0x80)
            val = base - step;
        else
            val = base + step;

        snprintf(&text[ypos++][45],40,"%-8s%02x  =  %02x + %02x  %-8s%02x",
                 "WavePos",wavepos,base,step,
                 "Val",val);
        snprintf(&text[ypos++][45],40,"%-6s%04x",
                 "Mod",V->LfoMod);
        break;
    }



}
