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
    if(S->ConfigFlags & S2X_CFG_SYSTEM1)
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

    ypos++;
    j = SCRN(ypos,44,25,"Tempo: %3d  Speed: %3d",T->BaseTempo,T->Tempo);
    ypos++;

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
                type = S->Voice[T->Channel[i].VoiceNo].Type;
                index = S->Voice[T->Channel[i].VoiceNo].Index;
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
            if(T->Channel[i].Enabled || T->Channel[i].LastEvent==2)
            {
                if(T->Channel[i].LastEvent==2)
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

static const char* fm_con_strings[] = {
    "",                                                 /* 0 */
    "\xda\xc4\xc4\xbf",                                 /* 1 /--\ */
    "\xc0M1\xc1\x43\x31\xc4M2\xc4\x43\x32\xc4",         /* 2 \M1'C1-M2-C2- */
    "\xc0M1\xc1\xc4\xc4\xc2M2\xc4\x43\x32\xc4",         /* 3 \M1'--,M2-C2- */
    "    \x43\x31\xd9",                                 /* 4     C1/       */
    "\xc0M1\xc1\xc4\xc4\xc4\xc4\xc4\xc2\x43\x32\xc4",   /* 5 \M1'-----,C2- */
    "    \x43\x31\xc4M2\xd9",                           /* 6     C1-M2/    */
    "\xc0M1\xc1\x43\x31\xc4\xc4\xc4\xc2\x43\x32\xc4",   /* 7 \M1'C1---,C2- */
    "       M2\xd9",                                    /* 8        M2/    */
    "\xc0M1\xc1\x43\x31\xc4\xc4\xc4\xc4\xc4\xc4\xc2",   /* 9 \M1'C1------, */
    "       M2\xc4\x43\x32\xd9",                        /*10        M2-C2/ */
    "\xc0M1\xc5\x43\x31\xc4\xc4\xc4\xc2\xc4\xc4\xc2",   /*11 \M1+C1---,--, */
    "   \xc3\xc4\xc4\xc4M2\xd9  \xb3",                  /*12    }---M2/  | */
    "   \xc0\xc4\xc4\xc4\xc4\xc4\xc4\x43\x32\xd9",      /*13    \------C2/ */
    "\xc0M1\xc1\x43\x31\xc4\xc4\xc4\xc2\xc4\xc4\xc2",   /*14 \M1'C1---,--, */
    "       M2\xd9  \xb3",                              /*15        M2/  | */
    "          \x43\x32\xd9",                           /*16           C2/ */
    "\xc0M1\xc1\xc4\xc4\xc2\xc4\xc4\xc2\xc4\xc4\xc2",   /*17 \M1'--,--,--, */
    "    \x43\x31\xd9  \xb3  \xb3",                     /*18     C1/  |  | */
};

static const char* fm_lfo_waveform[4] = {
    "SAW","SQU","TRI","NOI"
};

static const struct {
    int8_t line[4];
    int8_t offset[4];
} fm_con_param[8] = {
    {{2,0,0,0},{0,0,0,0}}, //0
    {{3,4,0,0},{0,1,0,0}}, //1
    {{5,6,0,0},{0,1,1,0}}, //2
    {{7,8,0,0},{0,0,1,0}}, //3
    {{9,10,0,0},{0,0,1,1}}, //4
    {{11,12,13,0},{0,0,1,2}}, //5
    {{14,15,16,0},{0,0,1,2}}, //6
    {{17,18,15,16},{0,1,2,3}}, //7
};

static void fm_con(int id,int ypos,int xpos)
{
    int i;
    SCRN(ypos++,xpos,14,"%s",fm_con_strings[1]);
    for(i=0;i<4;i++)
        SCRN(ypos+i,xpos,14,"%s",fm_con_strings[fm_con_param[id].line[i]]);
    for(i=0;i<4;i++)
        set_color(ypos+fm_con_param[id].offset[i],1+xpos+(i*3),1,2,COLOR_L_GREY,COLOR_D_BLUE);
    SCRN(ypos+3,xpos,8,"CON:%d",id);
}
/*
    instrument data format (very simple):
    0x00     : Reg 0x20 Channel control (FB, Connect)
    0x01     : Not used
    0x02     : Not used
    0x03     : Reg 0x38 PMS/AMS
    0x04-0x07: Reg 0x40 Operator DT1/MUL
    0x08-0x0b: Reg 0x60 Operator TL
    0x0c-0x0f: Reg 0x80 Operator KS/AR
    0x10-0x13: Reg 0xa0 Operator AME/D1R
    0x14-0x17: Reg 0xc0 Operator DT2/D2R
    0x18-0x1b: Reg 0xe0 Operator D1L/RR
    0x1c-0x1f: Not used
*/

static int fm_op(uint8_t* insdat,int ypos,char* title)
{
    uint8_t dt1 = (insdat[0x04]>>4)&7;
    uint8_t mul = insdat[0x04]&15;
    uint8_t tl  = insdat[0x08]&127;
    uint8_t ks  = insdat[0x0c]>>6;
    uint8_t ar  = insdat[0x0c]&31;
    uint8_t ase = insdat[0x10]>>7;
    uint8_t d1r = insdat[0x10]&31;
    uint8_t dt2 = insdat[0x14]>>6;
    uint8_t d2r = insdat[0x14]&15;
    uint8_t d1l = insdat[0x18]>>4;
    uint8_t rr  = insdat[0x18]&15;

    SCRN(ypos++,44,40,"%-31s%s",
         title,
         ase ? "ase" : "");
    SCRN(ypos++,44,40,"%4s:%02d%5s:%02d%4s:%02d%4s:%02d%3s:%02d",
         "AR",ar,"D1R",d1r,"D1L",d1l,"D2R",d2r,"RR",rr);
    SCRN(ypos++,44,40,"%4s:%03d%4s:%02d%4s:%02d%4s:%02d%3s:%02d",
         "TL",tl,"DT1",dt1,"DT2",dt2,"MUL",mul,"KS",ks);
    return ypos;
}


static int fm_info(S2X_State* S,uint8_t* insdat,int ypos)
{
    uint8_t fb_con = insdat[0];
    uint8_t pms_ams = insdat[3];

    SCRN(ypos++,44,40,"Chip registers:");
    fm_con(fb_con&7,ypos,45);
    SCRN(ypos++,44+14,40,"FB :%d    LFO Set:  %02x",(fb_con>>3)&7,S->FMLfo);
    SCRN(ypos++,44+14,40,"\x10           Wave: %s",fm_lfo_waveform[S->FMLfoWav&3]);
    SCRN(ypos++,44+14,40,"            Freq: %3d",S->FMLfoFrq);
    SCRN(ypos++,44+14,40,"PMS:%d    Amp Mod: %3d",(pms_ams>>4)&7,S->FMLfoAms);
    SCRN(ypos++,44+14,40,"AMS:%d    Phs Mod: %3d",pms_ams&3,S->FMLfoPms);
    ypos++;
    ypos = fm_op(insdat,ypos,"Modulator 1");
    ypos = fm_op(insdat+2,ypos,"Carrier 1");
    ypos = fm_op(insdat+1,ypos,"Modulator 2");
    ypos = fm_op(insdat+3,ypos,"Carrier 2");
    return ++ypos;
}

void ui_info_s2_voice(int id,int ypos)
{
    int tempypos;

    S2X_State *S = DriverInterface->Driver;

    int type = S->Voice[id].Type;
    int index = S->Voice[id].Index;

    S2X_PCMVoice* PCM = &S->PCM[index];
    S2X_FMVoice* FM = &S->FM[index];
    S2X_WSGVoice* WSG = &S->WSG[index];

    int flag;
    int trs=0, note=0;
    int8_t trs2=0;
    char* tempstr;
    struct S2X_Pitch* P = NULL;
    S2X_Channel* C = NULL;

    set_color(ypos,44,43,35,COLOR_D_BLUE,COLOR_L_GREY);

    if(type == S2X_VOICE_TYPE_PCM || type ==S2X_VOICE_TYPE_SE || type==S2X_VOICE_TYPE_WSG)
    {
        //vno = S2X_VOICE_TYPE_SE ? 24+index : index;

        SCRN(ypos++,44,40,"Chip registers:");
        SCRN(ypos++,44,40,"%6s:  %04x%6s:%04x%6s:%04x",
                 "Flag",    S->PCMChip.v[id].flags,
                 "VolF",    S->PCMChip.v[id].vol_f,
                 "VolR",    S->PCMChip.v[id].vol_r);
        SCRN(ypos++,44,40,"%6s:%06x%6s:%04x (%5.0f Hz)",
                 "Pos",     S->PCMChip.v[id].pos & 0xffffff,
                 "Freq",    S->PCMChip.v[id].freq,
                 ((double)S->PCMChip.v[id].freq/0x10000)*S->PCMChip.rate);
        SCRN(ypos++,44,40,"%6s:%02x%04x%6s:%04x%6s:%04x",
                 "Start",   S->PCMChip.v[id].wave_bank&0xff,S->PCMChip.v[id].wave_start,
                 "End",     S->PCMChip.v[id].wave_end,
                 "Loop",    S->PCMChip.v[id].wave_loop);
        ypos++;
    }
    else if(type == S2X_VOICE_TYPE_FM)
    {
        uint8_t fmdata[32] = {0};
        if(FM->InsPtr)
            memcpy(fmdata,S->Data+FM->InsPtr,32);
        ypos = fm_info(S,fmdata,ypos);
    }

    tempypos=ypos+1;

    switch(type)
    {
    default:
        return;
    case S2X_VOICE_TYPE_PCM:
        if(!PCM->Channel)
            return;

        if(PCM->Length)
            SCRN(ypos,60,40,"(Time Left:%3d/%3d)",PCM->Length,PCM->Channel->Vars[S2X_CHN_GTM]);

        flag = PCM->Flag;

        SCRN(ypos++,44,40,"Voice %s",flag&0x80 ? "Enabled":"Disabled");
        tempypos=ypos;

        SCRN(ypos++,45,40,"%-10s%04x (%4d)",
                 "WaveNo",      PCM->WaveNo,PCM->WaveNo);
        SCRN(ypos++,45,40,"%-10s%04x (%02x, %04x)",
                 "Envelope",    PCM->EnvNo,PCM->EnvNo,PCM->EnvValue);
        SCRN(ypos++,45,40,"%-10s%s",
                 "Status",      (flag&0x10) ? "key on" : "key off");
        SCRN(ypos++,45,40,"%-10s%4d (%4d,%4d)",
                 "Volume",      PCM->Volume,
                 PCM->Channel->Vars[S2X_CHN_VOL],PCM->Track->TrackVolume);
        SCRN(ypos++,45,40,"%-10s%4d (%4d)",
                 "Pan",         PCM->Pan,(int8_t)(PCM->Pan-128));
        SCRN(ypos++,45,40,"%-10s%4d (%4d)",
                 "PanSlide",    PCM->Channel->Vars[S2X_CHN_PANENV],(int8_t)PCM->PanSlide);
        P = &PCM->Pitch;
        C = PCM->Channel;
        break;
    case S2X_VOICE_TYPE_FM:
        if(!FM->Channel)
            return;

        if(FM->Length)
            SCRN(ypos,60,40,"(Time Left:%3d/%3d)",FM->Length,FM->Channel->Vars[S2X_CHN_GTM]);

        flag = FM->Flag;

        SCRN(ypos++,44,40,"Voice %s",flag&0x80 ? "Enabled":"Disabled");

        SCRN(ypos++,45,40,"%-10s%04x (%4d)",
                 "InsNo",       FM->InsNo,FM->InsNo);
        SCRN(ypos++,45,40,"%-10s%s",
                 "Status",      (flag&0x10) ? "key on" : "key off");
        SCRN(ypos++,45,40,"%-10s%4d (%4d,%4d)",
                 "Volume",      (S->ConfigFlags & S2X_CFG_FM_VOL) ? FM->Volume : ((~FM->Volume)&0x7f)<<1,
                 FM->Channel->Vars[S2X_CHN_VOL],FM->Track->TrackVolume);

        switch(FM->Channel->Vars[S2X_CHN_PAN]&0xc0)
        {
        default:
            tempstr = "(no output)";
            break;
        case 0x40:
            tempstr = "left"; // Left
            break;
        case 0x80:
            tempstr = "right";
            break;
        case 0xc0:
            tempstr = "center";
            break;
        }

        SCRN(ypos++,45,40,"%-10s%s",
                 "Pan",         tempstr);

        P = &FM->Pitch;
        C = FM->Channel;
        trs = 4;
        break;
    case S2X_VOICE_TYPE_WSG:

        if(!WSG->Channel)
            return;
        S2X_WSGChannel *W = &WSG->Channel->WSG;

        SCRN(ypos++,44,40,"Voice %s",W->Active ? "Enabled":"Disabled");

        SCRN(ypos++,45,40,"%-10s%02x",
                "WaveNo",      WSG->WaveNo);
        SCRN(ypos++,45,40,"%-10s%s",
                "Mode",        W->Noise ? "Noise" : "Tone");
        SCRN(ypos++,45,40,"%-10s%02x",
                "PitchTab",    W->PitchNo);
        SCRN(ypos++,45,40,"%-10s%02x (%04x, %02x)",
                "EnvelopeL",   W->Env[1].No,W->Env[1].Pos,W->Env[1].Val);
        SCRN(ypos++,45,40,"%-10s%02x (%04x, %02x)",
                "EnvelopeR",   W->Env[0].No,W->Env[0].Pos,W->Env[0].Val);
        SCRN(ypos++,45,40,"%-10s%08x",
                "Freq",        W->Freq);
        SCRN(ypos++,45,40,"%-10s%04x",
                "SeqPos",      W->SeqPos);
        SCRN(ypos++,45,40,"%-10s%02x",
                "SeqRepeat",   W->SeqRepeat);
        SCRN(ypos++,45,40,"%-10s%02x",
                "SeqLoop",     W->SeqLoop);
        SCRN(ypos++,45,40,"%-10s%02x (%02x)",
                "Tempo",       W->Tempo, W->SeqWait);
        return;
    }

    note = (P->Target>>8)+trs;
    trs2 = C->Vars[S2X_CHN_TRS];

    SCRN(ypos++,45,40,"%-10s %s%d (%+4d = %s%d)",
             "Note",Q_NoteNames[note%12],  (note-3)/12,
             (int8_t)C->Vars[S2X_CHN_TRS], Q_NoteNames[abs(note+trs2)%12], (note+trs2-3)/12);
    SCRN(ypos++,45,40,"%-10s%4d",
             "Detune",C->Vars[S2X_CHN_DTN]);
    SCRN(ypos++,45,40,"%-10s%4d (%04x)",
             "Porta",C->Vars[S2X_CHN_PTA],(uint16_t)(P->Value-P->Target));
    SCRN(ypos++,45,40,"%-10s%04x (%4d, pos: %06x)",
             "PitchEnv",P->EnvNo,P->EnvNo,P->EnvPos);
    SCRN(ypos++,45,40,"%-10s%4d",
             "EnvRate",P->EnvSpeed);
    SCRN(ypos++,45,40,"%-10s%4d (vol: %03d)",
             "EnvDepth",P->EnvDepth, P->VolDepth);
    SCRN(ypos++,45,40,"%-10s%04x (vol: %02x)",
             "EnvMod",P->EnvMod,P->VolDepth ? P->VolBase-P->VolMod : 0);

    if(~flag&0x80)
    {
        set_color(tempypos,44,ypos-tempypos,35,COLOR_D_BLUE,COLOR_D_GREY);
    }

    //ypos+=2;
}
