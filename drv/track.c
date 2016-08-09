/*
    Quattro - track functions
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "quattro.h"
#include "track.h"
#include "track_cmd.h"
#include "voice.h"
#include "helper.h"

// Call 0x06 - initializes a track
// source: 0x4baa
void Q_TrackInit(Q_State* Q, int TrackNo)
{
    Q->SongTimer[TrackNo] = 0;

    Q_Track* T = &Q->Track[TrackNo];

    if(T->Flags & Q_TRACK_STATUS_BUSY)
        Q_TrackStop(Q,TrackNo);

    uint16_t SongNo = Q->SongRequest[TrackNo]&0x7ff;

    T->Position = Q_GetSongPos(Q,SongNo);
    if(T->Position>0x7ffff)
    {
        Q_DEBUG("Track %02x, song id %04x invalid\n",TrackNo,SongNo);
        Q->SongRequest[TrackNo] &= ~(Q_TRACK_STATUS_START);
        return;
    }

    T->Flags = Q->SongRequest[TrackNo];

    if(!(T->Flags & Q_TRACK_STATUS_SUB))
        Q->ParentSong[TrackNo] = TrackNo;

    // Set track vars...
    T->Flags = (T->Flags&~(Q_TRACK_STATUS_START))|Q_TRACK_STATUS_BUSY;
    Q->SongRequest[TrackNo] = T->Flags;

    Q_DEBUG("T=%02x playing song %04x at %06x\n",TrackNo,SongNo,T->Position);

    T->SubStackPos=0;
    memset(T->SubStack,0,sizeof(T->SubStack));
    T->RepeatStackPos=0;
    memset(T->RepeatStack,0,sizeof(T->RepeatStack));
    T->LoopStackPos=0;
    memset(T->LoopStack,0,sizeof(T->LoopStack));

    T->SkipTrack = Q->BootSong > 1 ? 1 : 0;

    T->UpdateTime = Q->FrameCnt;
    T->Fadeout = 0;
    T->Unused1 = 0;
    T->RestCount = 0;
    T->SubStackPos = 0;
    T->LoopStackPos = 0;

    T->Tempo=8;
    T->BaseTempo=32;
    T->TempoMode=0;
    T->TempoFraction=0;
    T->TempoMulFactor=0;

    T->VolumeSource=&T->TrackVolume;

    int i;
    for(i=0;i<Q_MAX_TRKCHN;i++)
    {
        T->Channel[i].Enabled = 0;
    }
}

// Call 0x16 - stops a track
// source: 0x4ef0
void Q_TrackStop(Q_State* Q,int TrackNo)
{
    int i;
    for(i=0;i<Q_MAX_TRACKS;i++)
    {
        if(Q->ParentSong[i] == TrackNo)
        {
            Q_TrackDisable(Q,i);
        }
    }

}

// Call 0x0a - updates a track
// source: 0x4fc4
void Q_TrackUpdate(Q_State* Q,int TrackNo)
{
    Q->SongTimer[TrackNo] += 1.0/120.0;

    Q_Track* T = &Q->Track[TrackNo];
    uint8_t Command;
    uint32_t TempoVar;
    //Q_TrackCommand* CommandFunc;

    if(~T->Flags & Q_TRACK_STATUS_BUSY)
        return Q_TrackDisable(Q,TrackNo);

    if((int16_t)(T->UpdateTime-Q->FrameCnt) > 0)
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
        T->TicksLeft = T->SkipTrack ? 50000 : 1000;

        while(T->TicksLeft)
        {
            T->TicksLeft--;

            Q_LoopDetectionCheck(Q,TrackNo,0);
            Command = Q_ReadByte(Q,T->Position++);

            if(Command&0x80)
            {
                T->RestCount = Command&0x7f;
                break;
            }
            else
            {
                Q_TrackCommandTable[Command&0x3f](Q,TrackNo,T,&T->Position,Command);
            }
        }
    }

    // Calculate tempo
    if(T->TempoReg)
    {
        // tempo source from register
        T->UpdateTime += *T->TempoSource;
    }
    else if(T->TempoMode == 0xff)
    {
        // multiply mode
        T->UpdateTime += T->Tempo*T->BaseTempo;
        T->TempoMode=0;
        TempoVar = ((T->Tempo)<<8)|(T->TempoFraction);
        TempoVar *= (T->TempoMulFactor)<<8;
        if(TempoVar & 0x80000000) // overflow
            T->Tempo = 0xff;
        else if(TempoVar < 0x800000) // underflow
            T->Tempo = 1;
        else
        {
            T->Tempo = (TempoVar>>23)&0xff;
            T->TempoFraction = (TempoVar>>15)&0xff;
            T->TempoMode=0xff;
        }
    }
    else if(T->TempoMode)
    {
        // tempo sequence
        T->TempoMode--; // decrease counter
        Command=0xff;
        do
        {
            Command = Q_ReadByte(Q,T->TempoSeqPos++);
            if(Command == 0xff) // loop
            {
                T->TempoSeqPos=T->TempoSeqStart;
                T->TempoMode=0xfe; // endless
            }
        } while(Command == 0xff);
        T->Tempo = Command;
        T->UpdateTime += T->Tempo*T->BaseTempo;
    }
    else
    {
        T->UpdateTime += T->Tempo*T->BaseTempo;
    }

    //printf("next update: %04x (Tempo:%d, BaseTempo:%d)\n",T->UpdateTime,T->Tempo,T->BaseTempo);

}

// calculate track volume incl fade out and attenuation
// source: 0x4b2c
void Q_TrackCalcVolume(Q_State* Q,int TrackNo)
{
    Q_Track* T = &Q->Track[TrackNo];

    uint16_t vol = *T->VolumeSource;

    if(T->Flags & Q_TRACK_STATUS_ATTENUATE)
        vol += Q->BaseAttenuation;
    else if(T->Flags & Q_TRACK_STATUS_FADE)
    {
        T->Fadeout += Q->BaseFadeout;
        if(T->Fadeout > 0xd000) // above threshold?
        {
            // Disable track
            Q->SongRequest[TrackNo] &= ~(Q_TRACK_STATUS_BUSY);
            return;
        }
        else
        {
            vol += (T->Fadeout>>8);
        }
    }

    // Fix overflow
    if(vol > 0xff)
        vol = 0xff;

    T->GlobalVolume = vol;
}

// disables a track (split from song command 0x15)
// source: 0x5c50
void Q_TrackDisable(Q_State *Q,int TrackNo)
{

    Q_Track* T = &Q->Track[TrackNo];
    Q_Channel* c;
    int i;
    for(i=0;i<Q_MAX_VOICES;i++)
    {
        Q->ChannelPriority[i][TrackNo].channel = 0;
        Q->ChannelPriority[i][TrackNo].priority = 0;
    }
    for(i=0;i<Q_MAX_TRKCHN;i++)
    {
        c = &T->Channel[i];
        if(c->Enabled)
        {
            c->Enabled=0;
            c->KeyOnType=0;

            // skip any remaining events
            c->Voice->CurrEvent = c->Voice->LastEvent;

            // older MCUs seem to have different behavior for cutoff mode.
            // don't know exactly when this was changed though
            if(Q->McuVer < Q_MCUVER_Q00)
            {
                // based on airco22b.
                // envelope key off
                if(c->Voice->EnvNo && !(c->CutoffMode & 0x7f))
                {
                    c->Voice->GateTimeLeft=1;
                }
                else
                {
                    // turn off voice if looped. otherwise we let the loop play to the end.
                    if(Q->Chip.v[c->VoiceNo].flags & C352_FLG_LOOP)
                        Q->Chip.v[c->VoiceNo].flags = 0;
                }
            }
            else
            {
                // based on sws2000
                // "rough" cutoff: disable output entirely
                if(c->CutoffMode & 0x7f)
                    Q->Chip.v[c->VoiceNo].flags = 0;
                // "smooth" cutoff for non-enveloped samples: just disable loop
                else if(c->Voice->EnvNo == 0)
                    Q->Chip.v[c->VoiceNo].flags &= ~(C352_FLG_LOOP);
                // "smooth" cutoff for enveloped samples: Set note length left to 0
                else
                    c->Voice->GateTimeLeft=1;
            }
            Q_VoiceClearChannel(Q,c->VoiceNo);

            // Switch to next track on the voice
            int next_track = 0;
            int next_channel = 0;
            if(Q_VoiceGetPriority(Q,c->VoiceNo,&next_track,&next_channel))
            {
                Q_DEBUG("Voice %02x free, Now enabling trk %02x ch %02x\n",c->VoiceNo,next_track,next_channel);
                Q_VoiceSetChannel(Q,c->VoiceNo,next_track,next_channel);
            }
        }
    }

    Q->SongRequest[TrackNo] &= ~(Q_TRACK_STATUS_BUSY);
    T->Flags &= Q_TRACK_STATUS_FADE; // prevent race condition
    Q->ParentSong[TrackNo] = Q_MAX_TRACKS;
    T->TicksLeft = 0;

    Q_DEBUG("Track %02x terminated\n",TrackNo);
    Q->BootSong = 0; // disable boot song flag
}

// Call 0x28 - clear channel
// source: 0x4eb2
void Q_ChannelClear(Q_State *Q,int TrackNo,int ChannelNo)
{
    Q_Channel *C = &Q->Track[TrackNo].Channel[ChannelNo];
    uint8_t VoiceNo = C->VoiceNo;

    memset(C,0,sizeof(Q_Channel));
    C->VoiceNo = VoiceNo;
}
