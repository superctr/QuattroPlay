#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"
#include "tables.h"

#define TRACKCOMMAND(__name) static void __name(S2X_State* S,int TrackNo,S2X_Track* T,uint8_t Command,int8_t CommandType)
#define LOGCMD Q_DEBUG("Trk %02x Pos %06x, Cmd: %02x (%s)\n",TrackNo,T->PositionBase+T->Position,Command,__func__)

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
    if(S->CJump)
        T->Flags |= 0x400;

    Q_DEBUG("T=%02x conditional jump (%staken)\n",TrackNo,(T->Flags & 0x400) ? "" : "not ");

    if(~T->Flags & 0x400)
    {
        T->Position+=2;
        T->Flags |= 0x400;
        return;
    }
    return tc_Jump(S,TrackNo,T,Command,CommandType);
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
    int i=0, temp=0, type;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);
    if((Command&0x3f)==0x21) // NA-1/2 bios has duplicate 'set voice number' commands
        Command=0x20 | (Command&0x40);
    type = (CommandType == -1) ? (Command&0x3f)-S2X_CHN_OFFSET : CommandType;
    while(mask)
    {
        if(mask&0x80)
        {
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            T->Channel[i].Vars[type] = temp;
            T->Channel[i].UpdateMask |= 1<<type;
            // voice set var function here
            S2X_VoiceCommand(S,&T->Channel[i],Command,temp);
            if(type == S2X_CHN_FRQ)
            {
                T->Channel[i].LastEvent=1;
                if(temp != 0xff && T->Channel[i].Vars[S2X_CHN_LEG])
                    --T->Channel[i].Vars[S2X_CHN_LEG];
            }
        }
        mask<<=1;
        i++;
    }
    // key-on
    if(type == S2X_CHN_FRQ)
        T->TicksLeft=0;
}

TRACKCOMMAND(tc_WriteChannelDummy)
{
    LOGCMD;
    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);
    int i=0;
#ifdef DEBUG
    int temp=0;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);
#endif
    Q_DEBUG("T=%02x cmd %02x=",TrackNo,Command&0x3f);
    for(i=0;i<8;i++)
    {
        if(mask&0x80)
        {
#ifdef DEBUG
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            Q_DEBUG("%03d%s",temp,(i==7)?"":",");
#else
            T->Position++;
#endif
        }
        else
        {
            Q_DEBUG("___%s",(i==7)?"":",");
        }
        mask<<=1;
    }
    Q_DEBUG("\n");
}

static void RequestWSG(S2X_State *S, int TrackNo, int ParentSong, int SongNo)
{
    S->SongRequest[TrackNo] = SongNo|(S2X_TRACK_STATUS_START | S2X_TRACK_STATUS_SUB);
    S->ParentSong[TrackNo] = ParentSong;
}

TRACKCOMMAND(tc_RequestWSG)
{
    LOGCMD;
    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);
    int i=0, j;
    int temp=0;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);
    for(i=0;i<8;i++)
    {
        if(mask&0x80)
        {
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            // Allocate a slot for the song ID.
            // The original sound driver does something completely
            // different but the result is fairly similar
            for(j=8;j<16;j++)   // first look for the same song ID
            {
                if((S->SongRequest[j]&0xff) == temp)
                {
                    RequestWSG(S,j,TrackNo,temp);
                    goto done;
                }
            }
            for(j=8;j<16;j++)   // then look for any free slot
            {
                if(!(S->SongRequest[j] & (S2X_TRACK_STATUS_START|S2X_TRACK_STATUS_BUSY)))
                {
                    RequestWSG(S,j,TrackNo,temp);
                    goto done;
                }
            }
            // no free slots available, we just pick one
            RequestWSG(S,i+8,TrackNo,temp);
        }
done:
        mask<<=1;
    }
    Q_DEBUG("\n");
}

void S2X_ChannelInit(S2X_State* S,S2X_Track* T,int TrackNo,int start,uint8_t mask)
{
    int temp,pos;
    int i=start;
    mask &= ~(T->InitFlag);
    T->InitFlag |= mask;
    while(mask)
    {
        if(mask&0x80)
        {
            S2X_ChannelClear(S,TrackNo,i&7);
            T->Channel[i&7].VoiceNo = i;
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

TRACKCOMMAND(tc_InitChannel)
{
    int i = (Command&1)<<3;

    if(SYSTEM1 || (TrackNo & 8)) // FM
        i = 24;
    else if((Command&0x3f) == 0x1a)
        i = 8;

    return S2X_ChannelInit(S,T,TrackNo,i,arg_byte(S,T->PositionBase,&T->Position));
}

TRACKCOMMAND(tc_InitChannelNA)
{
    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);
    int i=0, temp=0, pri=0, voiceno;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);

    while(mask)
    {
        if(mask&0x80)
        {
            S2X_ChannelClear(S,TrackNo,i&7);
            T->Channel[i&7].VoiceNo = voiceno = (T->Channel[i].Vars[S2X_CHN_VNO])&0x0f;
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            S2X_VoiceSetPriority(S,voiceno,TrackNo,i&7,temp);
            // check for other tracks
            pri = S2X_VoiceGetPriority(S,voiceno,NULL,NULL);
            // no higher priority tracks on the voice? if so, allocate
            if(pri == temp)
            {
                S2X_VoiceSetChannel(S,voiceno,TrackNo,i&7);
                S2X_VoiceClear(S,voiceno);
            }
        }
        mask<<=1;
        i++;
    }
}

TRACKCOMMAND(tc_WriteComm)
{
    LOGCMD;
#if DEBUG
    int i = arg_byte(S,T->PositionBase,&T->Position);
    int temp = arg_byte(S,T->PositionBase,&T->Position);
    Q_DEBUG("Communication byte set %02x: %02x\n",i,temp);
#else
    T->Position+=2;
#endif
}

TRACKCOMMAND(tc_WriteCommS1)
{
    LOGCMD;
#if DEBUG
    int temp = arg_byte(S,T->PositionBase,&T->Position);
    Q_DEBUG("Communication byte set: %02x\n",temp);
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

            T->Channel[i&7].Vars[S2X_CHN_VOF]=temp+1; // for display
            T->Channel[i&7].LastEvent=2;
            S->SE[i&7].Track = TrackNo;
        }
        mask<<=1;
        i++;
    }
    // key-on
    if((Command&0x3f) == 0x1b)
        T->TicksLeft=0;
}

TRACKCOMMAND(tc_PercussionNA)
{
    uint8_t mask = arg_byte(S,T->PositionBase,&T->Position);
    int i=0, temp=0, voice=0;
    if(Command&0x40)
        temp = arg_byte(S,T->PositionBase,&T->Position);
    while(mask)
    {
        if(mask&0x80)
        {
            if(~Command&0x40)
                temp = arg_byte(S,T->PositionBase,&T->Position);
            // play sample on channel i
            if(T->Channel[i].Enabled || T->Channel[i].VoiceNo==0xff)
            {
                if(T->Channel[i].VoiceNo==0xff)
                    voice = 8+i;
                else
                    voice = T->Channel[i].VoiceNo;
                S->PCM[voice].Flag=0;
                S2X_PlayPercussion(S,voice,T->PositionBase,temp,(T->Fadeout)>>8);

                T->Channel[i].Vars[S2X_CHN_VOF]=temp+1; // for display
                T->Channel[i].LastEvent=2;
                S->SE[voice&7].Track = TrackNo;
            }
        }
        mask<<=1;
        i++;
    }
    // key-on
    if((Command&0x3f) == 0x1b)
        T->TicksLeft=0;
}


TRACKCOMMAND(tc_SetBank)
{
    uint8_t id = arg_byte(S,T->PositionBase,&T->Position);
    uint8_t val = arg_byte(S,T->PositionBase,&T->Position);

    id = S2X_NABankTable[id];
    S->WaveBank[id] = val<<1;
}

TRACKCOMMAND(tc_Dummy)
{
    LOGCMD;
    int max=8;
    if(CommandType>0)
        max=CommandType-1;
#ifdef DEBUG
    int i;
    if(max>0)
    {
        Q_DEBUG("args: %02x",arg_byte(S,T->PositionBase,&T->Position));
        for(i=0;i<max-1;i++)
            Q_DEBUG(", %02x",arg_byte(S,T->PositionBase,&T->Position));
        Q_DEBUG("\n");
    }
#else
    T->Position+=max;
#endif
    if(CommandType==-1)
    {
        QP_LoopDetectStop(&S->LoopDetect,TrackNo);
        S2X_TrackDisable(S,TrackNo);
    }
}

// system 2/21 command table
struct S2X_TrackCommandEntry S2X_S2TrackCommandTable[S2X_MAX_TRKCMD] =
{
/* 00 */ {1,-1,tc_Nop},
/* 01 */ {2,-1,tc_TrackVol},
/* 02 */ {2,-1,tc_Tempo},
/* 03 */ {2,-1,tc_Speed},
/* 04 */ {S2X_CMD_CALL,-1,tc_JumpSub},
/* 05 */ {S2X_CMD_RET,-1,tc_Return},
/* 06 */ {S2X_CMD_FRQ,-1,tc_WriteChannel}, // key on
/* 07 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // wave
/* 08 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume
/* 09 */ {S2X_CMD_JUMP,-1,tc_Jump},
/* 0a */ {S2X_CMD_REPT,-1,tc_Repeat},
/* 0b */ {S2X_CMD_LOOP,-1,tc_Loop},
/* 0c */ {S2X_CMD_TRS,-1,tc_WriteChannel}, // transpose
/* 0d */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // detune
/* 0e */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env no
/* 0f */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env speed
/* 10 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // legato
/* 11 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // gate time
/* 12 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env depth
/* 13 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume env
/* 14 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // delay
/* 15 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // lfo
/* 16 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pan
/* 17 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pan envelope
/* 18 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // link mode
/* 19 */ {S2X_CMD_CJUMP,-1,tc_CJump},
/* 1a */ {2,-1,tc_InitChannel}, // init voice (8-15) variant
/* 1b */ {S2X_CMD_WAV,-1,tc_Percussion},
/* 1c */ {3,-1,tc_WriteComm},
/* 1d */ {3,-1,tc_RequestTrack}, // FM
/* 1e */ {3,-1,tc_RequestTrack}, // PCM
/* 1f */ {S2X_CMD_WAV,-1,tc_Percussion},
/* 20 */ {2,-1,tc_InitChannel}, // init voice (0-7)
/* 21 */ {2,-1,tc_InitChannel}, // init voice (8-15)
/* 22 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // portamento
/* 23 */ {1,-1,tc_Nop},
/* 24 */ {1,-1,tc_Nop},
};

// NA-1/NA-2 command table
struct S2X_TrackCommandEntry S2X_NATrackCommandTable[S2X_MAX_TRKCMD] =
{
/* 00 */ {1,-1,tc_Nop},
/* 01 */ {2,-1,tc_TrackVol},
/* 02 */ {2,-1,tc_Tempo},
/* 03 */ {2,-1,tc_Speed},
/* 04 */ {S2X_CMD_CALL,-1,tc_JumpSub},
/* 05 */ {S2X_CMD_RET,-1,tc_Return},
/* 06 */ {S2X_CMD_FRQ,-1,tc_WriteChannel}, // key on
/* 07 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // wave
/* 08 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume
/* 09 */ {S2X_CMD_JUMP,-1,tc_Jump},
/* 0a */ {S2X_CMD_REPT,-1,tc_Repeat},
/* 0b */ {S2X_CMD_LOOP,-1,tc_Loop},
/* 0c */ {S2X_CMD_TRS,-1,tc_WriteChannel}, // transpose
/* 0d */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // detune
/* 0e */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env no
/* 0f */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env speed
/* 10 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // legato
/* 11 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // gate time
/* 12 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env depth
/* 13 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume env
/* 14 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // delay
/* 15 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // lfo
/* 16 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pan
/* 17 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pan envelope
/* 18 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // link mode
/* 19 */ {S2X_CMD_CJUMP,-1,tc_CJump},
/* 1a */ {1,-1,tc_Nop},
/* 1b */ {S2X_CMD_WAV,-1,tc_PercussionNA},
/* 1c */ {3,-1,tc_WriteComm},
/* 1d */ {3,-1,tc_RequestTrack}, // FM
/* 1e */ {3,-1,tc_RequestTrack}, // PCM
/* 1f */ {S2X_CMD_WAV,-1,tc_PercussionNA},
/* 20 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // set voice number
/* 21 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // ^
/* 22 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // portamento
/* 23 */ {3,-1,tc_SetBank},
/* 24 */ {S2X_CMD_CHN,-1,tc_InitChannelNA}, // set priority & initialize channel
};

// system 1 command table (tankfrce)
struct S2X_TrackCommandEntry S2X_S1AltTrackCommandTable[S2X_MAX_TRKCMD] =
{
/* 00 */ {1,-1,tc_Nop},
/* 01 */ {2,-1,tc_TrackVol},
/* 02 */ {2,-1,tc_Tempo},
/* 03 */ {2,-1,tc_Speed},
/* 04 */ {S2X_CMD_CALL,-1,tc_JumpSub},
/* 05 */ {S2X_CMD_RET,-1,tc_Return},
/* 06 */ {S2X_CMD_FRQ,-1,tc_WriteChannel}, // key on
/* 07 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // patch number
/* 08 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume
/* 09 */ {S2X_CMD_JUMP,-1,tc_Jump},
/* 0a */ {S2X_CMD_REPT,-1,tc_Repeat},
/* 0b */ {S2X_CMD_LOOP,-1,tc_Loop},
/* 0c */ {S2X_CMD_TRS,-1,tc_WriteChannel}, // transpose
/* 0d */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // detune
/* 0e */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env no
/* 0f */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env speed
/* 10 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // legato
/* 11 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // gate time
/* 12 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pitch env depth
/* 13 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume env
/* 14 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // delay
/* 15 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // lfo
/* 16 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // pan
/* 17 */ {S2X_CMD_CHN,-1,tc_WriteChannelDummy}, // not used
/* 18 */ {S2X_CMD_CHN,-1,tc_WriteChannelDummy}, // not used
/* 19 */ {3,-1,tc_RequestTrack}, // FM
/* 1a */ {1,-1,tc_Nop},
/* 1b */ {1,-1,tc_Nop},
/* 1c */ {3,-1,tc_WriteComm},
/* 1d */ {3,-1,tc_RequestTrack}, // FM
/* 1e */ {3,-1,tc_RequestTrack}, // FM
/* 1f */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // not used
/* 20 */ {2,-1,tc_InitChannel},
/* 21 */ {2,-1,tc_InitChannel},
/* 22 */ {1,-1,tc_Nop}, // not used
/* 23 */ {1,-1,tc_Nop},
/* 24 */ {1,-1,tc_Nop},
};

// system 1 command table (shadowld)
// i have not disassembled a sound driver yet, so this needs further investigation...
struct S2X_TrackCommandEntry S2X_S1TrackCommandTable[S2X_MAX_TRKCMD] =
{
/* 00 */ {1,-1,tc_Nop},
/* 01 */ {2,-1,tc_TrackVol},
/* 02 */ {2,-1,tc_Tempo},
/* 03 */ {2,-1,tc_Speed},
/* 04 */ {S2X_CMD_CALL,-1,tc_JumpSub},
/* 05 */ {S2X_CMD_RET,-1,tc_Return},
/* 06 */ {S2X_CMD_FRQ,-1,tc_WriteChannel}, // key on
/* 07 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // patch number
/* 08 */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // volume
/* 09 */ {S2X_CMD_JUMP,-1,tc_Jump},
/* 0a */ {S2X_CMD_END,-1,tc_Dummy}, //???
/* 0b */ {S2X_CMD_REPT,-1,tc_Repeat},
/* 0c */ {S2X_CMD_LOOP,-1,tc_Loop},
/* 0d */ {S2X_CMD_TRS,S2X_CHN_TRS,tc_WriteChannel}, // transpose
/* 0e */ {S2X_CMD_CHN,S2X_CHN_DTN,tc_WriteChannel}, // detune
/* 0f */ {S2X_CMD_CHN,S2X_CHN_PIT,tc_WriteChannel}, // pitch env no
/* 10 */ {S2X_CMD_CHN,S2X_CHN_PITRAT,tc_WriteChannel}, // pitch env speed
/* 11 */ {S2X_CMD_CHN,S2X_CHN_LEG,tc_WriteChannel}, // legato
/* 12 */ {S2X_CMD_CHN,S2X_CHN_GTM,tc_WriteChannel}, // gate time
/* 13 */ {S2X_CMD_CHN,S2X_CHN_PITDEP,tc_WriteChannel}, // pitch env depth
/* 14 */ {S2X_CMD_CHN,S2X_CHN_ENV,tc_WriteChannel}, // volume env
/* 15 */ {S2X_CMD_CHN,S2X_CHN_DEL,tc_WriteChannel}, // delay, but it works different?
/* 16 */ {S2X_CMD_CHN,S2X_CHN_C18,tc_RequestWSG}, // plays a track on the WSG
/* 17 */ {S2X_CMD_CHN,S2X_CHN_C18,tc_WriteChannelDummy}, // play a DAC sample
/* 18 */ {S2X_CMD_CHN,S2X_CHN_LFO,tc_WriteChannel}, // lfo
/* 19 */ {S2X_CMD_CHN,S2X_CHN_PAN,tc_WriteChannel}, // pan
/* 1a */ {1,-1,tc_Nop},
/* 1b */ {2,-1,tc_WriteCommS1},
/* 1c */ {2, 2,tc_Dummy}, // shadowld song 0e: song request
/* 1d */ {3,-1,tc_RequestTrack}, // FM
/* 1e */ {3,-1,tc_RequestTrack}, // FM
/* 1f */ {S2X_CMD_CHN,-1,tc_WriteChannel}, // not used
/* 20 */ {2,-1,tc_InitChannel},
/* 21 */ {2,-1,tc_InitChannel},
/* 22 */ {1,-1,tc_Nop}, // not used
/* 23 */ {1,-1,tc_Nop},
/* 24 */ {1,-1,tc_Nop},
};

struct S2X_TrackCommandEntry* S2X_TrackCommandTable[S2X_TYPE_MAX] =
{
    S2X_S2TrackCommandTable,
    S2X_S1TrackCommandTable,
    S2X_S1AltTrackCommandTable,
    S2X_NATrackCommandTable // NA
};
