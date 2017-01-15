#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../qp.h"
#include "helper.h"
#include "track.h"
#include "voice.h"

#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)

void S2X_TrackInit(S2X_State* S, int TrackNo)
{
    S->SongTimer[TrackNo] = 0;

    S2X_Track* T = &S->Track[TrackNo];

    if(T->Flags & S2X_TRACK_STATUS_BUSY)
        S2X_TrackStop(S,TrackNo);

    uint16_t SongNo = S->SongRequest[TrackNo]&0x1ff;

    //T->Position = S2X_GetSongPos(S,SongNo);
    T->PositionBase = (TrackNo > 7) ? S->FMBase : S->PCMBase;
    T->PositionBase += (SongNo & 0x100) ? 0x20000 : 0;

    T->Position = S2X_ReadWord(S,T->PositionBase+S2X_ReadWord(S,T->PositionBase)+(2*(SongNo&0xff)));
    Q_DEBUG("track pos=%04x,%04x\n",S2X_ReadWord(S,T->PositionBase),T->Position);

    // the sound driver does a check to make sure first byte is either 0x20 or 0x21
    uint8_t header_byte = S2X_ReadByte(S,T->PositionBase+T->Position);
    if(header_byte != 0x20 && header_byte != 0x21 && header_byte != 0x1a)
    {
        Q_DEBUG("Track %02x, song id %04x invalid\n",TrackNo,SongNo);
        S->SongRequest[TrackNo] &= ~(S2X_TRACK_STATUS_START);
        return;
    }

    T->Flags = S->SongRequest[TrackNo];

    if(!(T->Flags & S2X_TRACK_STATUS_SUB))
        S->ParentSong[TrackNo] = TrackNo;

    // Set track vars...
    T->Flags = (T->Flags&~(S2X_TRACK_STATUS_START))|S2X_TRACK_STATUS_BUSY;
    S->SongRequest[TrackNo] = T->Flags;

    Q_DEBUG("T=%02x playing song %04x at %06x\n",TrackNo,SongNo,T->PositionBase+T->Position);

    QP_LoopDetectStart(&S->LoopDetect,TrackNo,S->ParentSong[TrackNo],SongNo);

    T->SubStackPos=0;
    memset(T->SubStack,0,sizeof(T->SubStack));
    T->RepeatStackPos=0;
    memset(T->RepeatStack,0,sizeof(T->RepeatStack));
    T->LoopStackPos=0;
    memset(T->LoopStack,0,sizeof(T->LoopStack));

    T->UpdateTime = S->FrameCnt;
    T->Fadeout = 0;
    T->RestCount = 0;
    T->SubStackPos = 0;
    T->LoopStackPos = 0;
    T->InitFlag = 0;

    T->Tempo=0;
    T->BaseTempo=1;

    int i;
    for(i=0;i<Q_MAX_TRKCHN;i++)
    {
        T->Channel[i].Enabled = 0;
    }
}

void S2X_TrackStop(S2X_State* S,int TrackNo)
{
    int i;
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        if(S->ParentSong[i] == TrackNo)
        {
            S2X_TrackDisable(S,i);
        }
    }

}

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

static void S2X_TrackReadCommand(S2X_State *S,int TrackNo,uint8_t Command)
{
    S2X_Track *T = &S->Track[TrackNo];
    //S2X_Channel *C;
    int i=-1, pos, temp;
    uint8_t mask;

    //Q_DEBUG("T=%02x parse cmd %02x at %06x\n",TrackNo,Command,T->PositionBase+T->Position-1);
    switch(Command&0x3f)
    {
    case 0x00:
        break;
    case 0x01:
        T->TrackVolume = arg_byte(S,T->PositionBase,&T->Position);
        break;
    case 0x02:
        T->BaseTempo = arg_byte(S,T->PositionBase,&T->Position);
        break;
    case 0x03:
        T->Tempo = arg_byte(S,T->PositionBase,&T->Position);
        break;
    case 0x04:
        temp = arg_word(S,T->PositionBase,&T->Position);
        // if calling the same subroutine inside itself, do not increase the stack
        if(T->SubStackPos && T->SubStack[T->SubStackPos-1] == T->Position)
        {
            T->Position = temp;
            break;
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
        break;
    case 0x05:
        if(T->SubStackPos > 0)
        {
            T->Position = T->SubStack[--T->SubStackPos];
            QP_LoopDetectPop(&S->LoopDetect,TrackNo);
            QP_LoopDetectJump(&S->LoopDetect,TrackNo,T->Position+T->PositionBase);
            break;
        }
        QP_LoopDetectStop(&S->LoopDetect,TrackNo);
        S2X_TrackDisable(S,TrackNo);
        break;
    case 0x19:
        if(SYSTEM1)
            goto sys1_sub;
        Q_DEBUG("T=%02x parse cmd 0x19 (flag=%04x)\n",TrackNo,T->Flags & 0x400);
        if(~T->Flags & 0x400)
        {
            T->Flags |= 0x400;
            break;
        }
    case 0x09: // jump
        T->Position = arg_word(S,T->PositionBase,&T->Position);
        QP_LoopDetectJump(&S->LoopDetect,TrackNo,T->PositionBase+T->Position);
        break;
    case 0x0a: // repeat
        i = arg_byte(S,T->PositionBase,&T->Position);
        temp = arg_word(S,T->PositionBase,&T->Position);
        pos = T->RepeatStackPos;

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
        break;
    case 0x0b: // loop
        i = arg_byte(S,T->PositionBase,&T->Position);
        temp = arg_word(S,T->PositionBase,&T->Position);
        pos = T->LoopStackPos;

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
        break;
    case 0x06: case 0x07: case 0x08:
    case 0x0c: case 0x0d: case 0x0e:
    case 0x0f: case 0x10: case 0x11:
    case 0x12: case 0x13: case 0x14:
    case 0x15: case 0x16: case 0x17:
    case 0x18: case 0x22:
        mask = arg_byte(S,T->PositionBase,&T->Position);
        i=0;
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
        break;
    case 0x20: // different syntax on NA-1/NA-2
    case 0x21:
        if(SYSTEM1 || (TrackNo & 8)) // FM
            i = 24;
        else // PCM
            i = (Command&1)<<3;
    case 0x1a:
        if(!SYSTEM1 && (Command&0x3f) == 0x1a)
            i = 8;

        mask = arg_byte(S,T->PositionBase,&T->Position);

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
        break;
    case 0x1c:
        i = arg_byte(S,T->PositionBase,&T->Position);
        temp = arg_byte(S,T->PositionBase,&T->Position);
        break;
    case 0x1d:
        if(!SYSTEM1)
            i = (arg_byte(S,T->PositionBase,&T->Position) & 0x07)+8;
sys1_sub:
    case 0x1e:
        if(i<0)
            i = arg_byte(S,T->PositionBase,&T->Position) & 0x07;

        temp = arg_byte(S,T->PositionBase,&T->Position);
        temp += S->SongRequest[TrackNo]&0x100;

        S->SongRequest[i] = temp| (S2X_TRACK_STATUS_START | S2X_TRACK_STATUS_SUB);
        S->ParentSong[i] = TrackNo;
        break;
    case 0x1b:
    case 0x1f:
        mask = arg_byte(S,T->PositionBase,&T->Position);
        i=S2X_MAX_VOICES_PCM;
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
        break;
    default:
        Q_DEBUG("unrecognized command %02x at %06x\n",Command,T->PositionBase+T->Position);
        S2X_TrackDisable(S,TrackNo);
        break;
    }
}

// Call 0x0a - updates a track
// source: 0x4fc4
void S2X_TrackUpdate(S2X_State* S,int TrackNo)
{
    S->SongTimer[TrackNo] += 1.0/120.0;

    S2X_Track* T = &S->Track[TrackNo];
    uint8_t Command;

    if(~T->Flags & S2X_TRACK_STATUS_BUSY)
        return S2X_TrackDisable(S,TrackNo);

    if((int16_t)(T->UpdateTime-S->FrameCnt) > 0)
        return;

    if(T->RestCount)
    {
        // we're resting...
        T->RestCount--;
    }
    else
    {
        // Parse commands...
        // max amount of commands before switching tracks... (Not a feature of the original driver)
        T->TicksLeft = 1000;

        while(T->TicksLeft)
        {
            T->TicksLeft--;

            QP_LoopDetectCheck(&S->LoopDetect,TrackNo,T->PositionBase+T->Position);

            Command = arg_byte(S,T->PositionBase,&T->Position);

            if(Command&0x80)
            {
                T->TicksLeft=0;
                T->RestCount = Command&0x7f;
            }
            else
                S2X_TrackReadCommand(S,TrackNo,Command);
        }
    }

    // Calculate tempo
    T->UpdateTime += T->Tempo*T->BaseTempo;
}

// calculate track volume incl fade out and attenuation
void S2X_TrackCalcVolume(S2X_State* S,int TrackNo)
{
    S2X_Track* T = &S->Track[TrackNo];

    if(T->Flags & S2X_TRACK_STATUS_ATTENUATE)
        T->Fadeout = 0x10;//S2X->BaseAttenuation;
    else if(T->Flags & S2X_TRACK_STATUS_FADE)
    {
        T->Fadeout += 0x40;//Q->BaseFadeout;
        if(T->Fadeout > 0xd000) // above threshold?
        {
            // Disable track
            S->SongRequest[TrackNo] &= ~(S2X_TRACK_STATUS_BUSY);
            return;
        }
    }
    else
        T->Fadeout = 0;
}

// disables a track (split from song command 0x15)
void S2X_TrackDisable(S2X_State *S,int TrackNo)
{
    S2X_Track* T = &S->Track[TrackNo];
    S2X_Channel* c;
    int i;
    //uint16_t cflags;
    for(i=0;i<S2X_MAX_VOICES;i++)
    {
        S->ChannelPriority[i][TrackNo].channel = 0;
        S->ChannelPriority[i][TrackNo].priority = 0;
    }
    for(i=0;i<S2X_MAX_TRKCHN;i++)
    {
        c = &T->Channel[i];
        if(c->Enabled)
        {
            c->Enabled=0;
            //c->KeyOnType=0;

            S2X_VoiceClearChannel(S,c->VoiceNo);
            S2X_VoiceClear(S,c->VoiceNo);

            // Switch to next track on the voice
            int next_track = 0;
            int next_channel = 0;
            if(S2X_VoiceGetPriority(S,c->VoiceNo,&next_track,&next_channel))
            {
                Q_DEBUG("Voice %02x free, Now enabling trk %02x ch %02x\n",c->VoiceNo,next_track,next_channel);
                S2X_VoiceSetChannel(S,c->VoiceNo,next_track,next_channel);

                S->Track[next_track].Channel[next_channel].UpdateMask=~((1<<S2X_CHN_PANENV)|(1<<S2X_CHN_FRQ));
                S2X_VoiceCommand(S,&S->Track[next_track].Channel[next_channel],0,0);
            }
        }
    }

    S->SongRequest[TrackNo] &= ~(S2X_TRACK_STATUS_BUSY);
    T->Flags &= S2X_TRACK_STATUS_FADE; // prevent race condition
    S->ParentSong[TrackNo] = S2X_MAX_TRACKS;
    T->TicksLeft = 0;

    Q_DEBUG("Track %02x terminated\n",TrackNo);
}

// Call 0x28 - clear channel
// source: 0x4eb2
void S2X_ChannelClear(S2X_State *S,int TrackNo,int ChannelNo)
{
    S2X_Channel *C = &S->Track[TrackNo].Channel[ChannelNo];
    uint8_t VoiceNo = C->VoiceNo;

    memset(C,0,sizeof(S2X_Channel));
    C->VoiceNo = VoiceNo;
    C->Track = &S->Track[TrackNo];
    C->Vars[S2X_CHN_LFO] = 0xff; // disables LFO
    C->Vars[S2X_CHN_PAN] = 0x80;
    C->Vars[S2X_CHN_WAV] = 0xff;
}
