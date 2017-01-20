#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"

#define TRACKCOMMAND(__name) static void __name(S2X_State* S,int TrackNo,S2X_Track* T,uint8_t Command)
#define LOGCMD Q_DEBUG("Trk %02x Pos %06x, Cmd: %02x (%s)\n",TrackNo,T->Position,Command,__func__)

#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)

static uint8_t arg_byte(S2X_State *S,int b,uint16_t* d)
{
    uint8_t r = S2X_ReadByte(S,b+*d);
    *d += 1;
    return r;
}
static uint16_t arg_word(S2X_State *S,int b,uint16_t* d)
{
    uint16_t r = S2X_ReadWord(S,b+*d);
    *d += 2;
    return r;
}

TRACKCOMMAND(tc_Nop)
{
    return;
}

TRACKCOMMAND(tc_TrackVol)
{
    T->TrackVolume = arg_byte(S,T->PositionBase,&T->Position);
}

TRACKCOMMAND(tc_Tempo)
{
    T->BaseTempo = arg_byte(S,T->PositionBase,&T->Position);
}

TRACKCOMMAND(tc_Speed)
{
    T->Tempo = arg_byte(S,T->PositionBase,&T->Position);
}

TRACKCOMMAND(tc_JumpSub)
{
    int temp = arg_word(S,T->PositionBase,&T->Position);
    // if calling the same subroutine inside itself, do not increase the stack
    if(T->SubStackPos && T->SubStack[T->SubStackPos-1] == T->Position)
    {
        T->Position = temp;
        return;
    }
    T->SubStack[T->SubStackPos++] = T->Position;
    T->Position = temp;

    if(T->SubStackPos == S2X_MAX_SUB_STACK)
    {
        Q_DEBUG("T=%02x smashed subroutine stack!\n",TrackNo);
        QP_LoopDetectStop(&S->LoopDetect,TrackNo);
        S2X_TrackDisable(S,TrackNo);
    }
    QP_LoopDetectPush(&S->LoopDetect,TrackNo);
    QP_LoopDetectJump(&S->LoopDetect,TrackNo,T->Position+T->PositionBase);
}

TRACKCOMMAND(tc_Return)
{
    if(T->SubStackPos > 0)
    {
        T->Position = T->SubStack[--T->SubStackPos];
        QP_LoopDetectPop(&S->LoopDetect,TrackNo);
        QP_LoopDetectJump(&S->LoopDetect,TrackNo,T->Position+T->PositionBase);
        return;
    }
    QP_LoopDetectStop(&S->LoopDetect,TrackNo);
    S2X_TrackDisable(S,TrackNo);
}

TRACKCOMMAND(tc_Jump)
{
    T->Position = arg_word(S,T->PositionBase,&T->Position);
    QP_LoopDetectJump(&S->LoopDetect,TrackNo,T->PositionBase+T->Position);
}

TRACKCOMMAND(tc_CJump)
{
    Q_DEBUG("T=%02x conditional jump (%staken)\n",TrackNo,(T->Flags & 0x400) ? "" : "not ");
    if(~T->Flags & 0x400)
    {
        T->Position+=2;
        T->Flags |= 0x400;
        return;
    }
    return tc_Jump(S,TrackNo,T,Command);
}

TRACKCOMMAND(tc_Repeat)
{
    int i = arg_byte(S,T->PositionBase,&T->Position);
    int temp = arg_word(S,T->PositionBase,&T->Position);
    int pos = T->RepeatStackPos;

    if(pos > 0 && T->RepeatStack[pos-1] == T->Position)
    {
        // loop address stored in stack
        pos--;
        if(--T->RepeatCount[pos] > 0)
            T->Position = temp;
        else
            T->RepeatStackPos=pos;
    }
    else
    {
        // new loop
        T->RepeatStack[pos] = T->Position;
        T->RepeatCount[pos] = i;
        T->RepeatStackPos++;
        T->Position = temp;

        if(T->RepeatStackPos == S2X_MAX_REPEAT_STACK)
        {
            Q_DEBUG("T=%02x smashed repeat stack!\n",TrackNo);
            QP_LoopDetectStop(&S->LoopDetect,TrackNo);
            S2X_TrackDisable(S,TrackNo);
        }
    }
}

TRACKCOMMAND(tc_Loop)
{
    int i = arg_byte(S,T->PositionBase,&T->Position);
    int temp = arg_word(S,T->PositionBase,&T->Position);
    int pos = T->LoopStackPos;

    if(pos > 0 && T->LoopStack[pos-1] == T->Position)
    {
        // loop address stored in stack
        pos--;
        if(--T->LoopCount[pos] == 0)
        {
            T->LoopStackPos=pos;
            T->Position = temp;
        }
    }
    else
    {
        // new loop
        T->LoopStack[pos] = T->Position;
        T->LoopCount[pos] = i;
        T->LoopStackPos++;

        if(T->LoopStackPos == S2X_MAX_LOOP_STACK)
        {
            Q_DEBUG("T=%02x smashed loop stack!\n",TrackNo);
            QP_LoopDetectStop(&S->LoopDetect,TrackNo);
            S2X_TrackDisable(S,TrackNo);
        }
    }
}

TRACKCOMMAND(tc_WriteChannel)
{
    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);
    int i=0, temp=0;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);

    while(mask)
    {
        if(mask&0x80)
        {
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            T->Channel[i].Vars[(Command&0x3f)-S2X_CHN_OFFSET] = temp;
            T->Channel[i].UpdateMask |= 1<<((Command&0x3f)-S2X_CHN_OFFSET);
            // voice set var function here
            S2X_VoiceCommand(S,&T->Channel[i],Command,temp);
        }
        mask<<=1;
        i++;
    }
    // key-on
    if((Command&0x3f) == 0x06)
        T->TicksLeft=0;
}

TRACKCOMMAND(tc_InitChannel)
{
    int temp,pos;
    int i = (Command&1)<<3;

    if(SYSTEM1 || (TrackNo & 8)) // FM
        i = 24;
    else if((Command&0x3f) == 0x1a)
        i = 8;

    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);

    mask &= ~T->InitFlag;
    T->InitFlag |= mask;
    while(mask)
    {
        if(mask&0x80)
        {
            //printf("voice %d to be allocated\n",i);
            S2X_ChannelClear(S,TrackNo,i&7);
            T->Channel[i&7].VoiceNo = i;
            //T->Channel[ChannelNo].Voice = &Q->Voice[VoiceNo];

            temp=100;

            S2X_VoiceSetPriority(S,i,TrackNo,i&7,temp);

            // check for other tracks
            pos = S2X_VoiceGetPriority(S,i,NULL,NULL);
            // no higher priority tracks on the voice? if so, allocate
            if(pos == temp)
            {
                S2X_VoiceSetChannel(S,i,TrackNo,i&7);
                S2X_VoiceClear(S,i);
            }

        }
        mask<<=1;
        i++;
    }
}

TRACKCOMMAND(tc_RequestSE)
{
    LOGCMD;
#if DEBUG
    int i = arg_byte(S,T->PositionBase,&T->Position);
    int temp = arg_byte(S,T->PositionBase,&T->Position);
    Q_DEBUG("SE request %02x: %02x\n",i,temp);
#else
    T->Position+=2;
#endif
}

TRACKCOMMAND(tc_RequestTrack)
{
    int i = arg_byte(S,T->PositionBase,&T->Position) & 0x07;
    if(!SYSTEM1 && (Command&0x3f)==0x1d)
        i+=8;

    int temp = arg_byte(S,T->PositionBase,&T->Position);
    temp += S->SongRequest[TrackNo]&0x100;

    S->SongRequest[i] = temp| (S2X_TRACK_STATUS_START | S2X_TRACK_STATUS_SUB);
    S->ParentSong[i] = TrackNo;
}

TRACKCOMMAND(tc_Percussion)
{
    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);
    int i=S2X_MAX_VOICES_PCM, temp=0;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);
    while(mask)
    {
        if(mask&0x80)
        {
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            // play sample on channel i
            S2X_PlayPercussion(S,i,T->PositionBase,temp,(T->Fadeout)>>8);
        }
        mask<<=1;
        i++;
    }
    // key-on
    if((Command&0x3f) == 0x1b)
        T->TicksLeft=0;
}

// command table (system2)
struct S2X_TrackCommandEntry S2X_S2TrackCommandTable[S2X_MAX_TRKCMD] =
{
/* 00 */ {1,tc_Nop},
/* 01 */ {2,tc_TrackVol},
/* 02 */ {2,tc_Tempo},
/* 03 */ {2,tc_Speed},
/* 04 */ {-5,tc_JumpSub},
/* 05 */ {-9,tc_Return},
/* 06 */ {-2,tc_WriteChannel}, // key on
/* 07 */ {-1,tc_WriteChannel}, // wave
/* 08 */ {-1,tc_WriteChannel}, // volume
/* 09 */ {-6,tc_Jump},
/* 0a */ {-7,tc_Repeat},
/* 0b */ {-8,tc_Loop},
/* 0c */ {-3,tc_WriteChannel}, // transpose
/* 0d */ {-1,tc_WriteChannel}, // detune
/* 0e */ {-1,tc_WriteChannel}, // pitch env no
/* 0f */ {-1,tc_WriteChannel}, // pitch env speed
/* 10 */ {-1,tc_WriteChannel}, // legato
/* 11 */ {-1,tc_WriteChannel}, // gate time
/* 12 */ {-1,tc_WriteChannel}, // pitch env depth
/* 13 */ {-1,tc_WriteChannel}, // volume env
/* 14 */ {-1,tc_WriteChannel}, // delay
/* 15 */ {-1,tc_WriteChannel}, // lfo
/* 16 */ {-1,tc_WriteChannel}, // pan
/* 17 */ {-1,tc_WriteChannel}, // pan envelope
/* 18 */ {-1,tc_WriteChannel}, // link mode
/* 19 */ {-4,tc_CJump},
/* 1a */ {2,tc_InitChannel}, // init voice (8-15) variant
/* 1b */ {-2,tc_Percussion},
/* 1c */ {3,tc_RequestSE},
/* 1d */ {3,tc_RequestTrack}, // FM
/* 1e */ {3,tc_RequestTrack}, // PCM
/* 1f */ {-2,tc_Percussion},
/* 20 */ {2,tc_InitChannel}, // init voice (0-7)
/* 21 */ {2,tc_InitChannel}, // init voice (8-15)
/* 22 */ {-1,tc_WriteChannel}, // portamento
/* 23 */ {1,tc_Nop},
/* 24 */ {1,tc_Nop},
};

// command table (system1)
struct S2X_TrackCommandEntry S2X_S1TrackCommandTable[S2X_MAX_TRKCMD] =
{
/* 00 */ {1,tc_Nop},
/* 01 */ {2,tc_TrackVol},
/* 02 */ {2,tc_Tempo},
/* 03 */ {2,tc_Speed},
/* 04 */ {-5,tc_JumpSub},
/* 05 */ {-9,tc_Return},
/* 06 */ {-2,tc_WriteChannel}, // key on
/* 07 */ {-1,tc_WriteChannel}, // wave
/* 08 */ {-1,tc_WriteChannel}, // volume
/* 09 */ {-6,tc_Jump},
/* 0a */ {-7,tc_Repeat},
/* 0b */ {-8,tc_Loop},
/* 0c */ {-3,tc_WriteChannel}, // transpose
/* 0d */ {-1,tc_WriteChannel}, // detune
/* 0e */ {-1,tc_WriteChannel}, // pitch env no
/* 0f */ {-1,tc_WriteChannel}, // pitch env speed
/* 10 */ {-1,tc_WriteChannel}, // legato
/* 11 */ {-1,tc_WriteChannel}, // gate time
/* 12 */ {-1,tc_WriteChannel}, // pitch env depth
/* 13 */ {-1,tc_WriteChannel}, // volume env
/* 14 */ {-1,tc_WriteChannel}, // delay
/* 15 */ {-1,tc_WriteChannel}, // lfo
/* 16 */ {-1,tc_WriteChannel}, // pan
/* 17 */ {-1,tc_WriteChannel}, // not used
/* 18 */ {-1,tc_WriteChannel}, // not used
/* 19 */ {3,tc_RequestTrack}, // FM
/* 1a */ {1,tc_Nop},
/* 1b */ {1,tc_Nop},
/* 1c */ {3,tc_RequestSE},
/* 1d */ {3,tc_RequestTrack}, // FM
/* 1e */ {3,tc_RequestTrack}, // FM (one of them is for WSG...)
/* 1f */ {-2,tc_WriteChannel}, // not used
/* 20 */ {2,tc_InitChannel},
/* 21 */ {2,tc_InitChannel},
/* 22 */ {1,tc_Nop}, // not used
/* 23 */ {1,tc_Nop},
/* 24 */ {1,tc_Nop},
};
