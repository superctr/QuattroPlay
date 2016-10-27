/*
    Quattro - Sound driver init & update functions...
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "quattro.h"
#include "helper.h"
#include "track.h"
#include "tables.h"
#include "update.h"

void Q_Init(Q_State *Q)
{
    Q_LoopDetectionInit(Q);
    Q_GetMcuVer(Q);
    Q_Reset(Q);
}

void Q_Deinit(Q_State *Q)
{
    Q_LoopDetectionFree(Q);
}

void Q_Reset(Q_State *Q)
{
    Q_LoopDetectionReset(Q);
    int i;

    memset(Q->Chip.v,0,sizeof(Q->Chip.v));
    memset(Q->Voice,0,sizeof(Q->Voice));

    for(i=0;i<Q_MAX_VOICES;i++)
    {
        Q_C352_W(Q,i,C352_FLAGS,0);
        if(Q->PortaFix)
            Q->Voice[i].Pitch = 0x3000; // A3 works well for ncv1.
    }

    Q->Chip.mute_mask=0;

    memset(Q->SongRequest,0,sizeof(Q->SongRequest));
    for(i=0;i<Q_MAX_TRACKS;i++)
    {
        Q->ParentSong[i] = Q_MAX_TRACKS;
        Q->SongTimer[i] = 0;
    }

    memset(Q->TrackParam,0,sizeof(Q->TrackParam));
    memset(Q->Register,0,sizeof(Q->Register));
    memset(Q->SongMessage,0,sizeof(Q->SongMessage));
    memset(Q->Track,0,sizeof(Q->Track));
    memset(Q->ChannelPriority,0,sizeof(Q->ChannelPriority));

    Q->BasePitch=0;

    Q->BaseFadeout=0x9a;        // add this to config ini?
    Q->BaseAttenuation=0x1c;
    Q->TrackCount=Q_MAX_TRACKS;
    Q->VoiceCount=Q_MAX_VOICES;

    Q->FrameCnt=0;
    Q->LFSR1 = 0x5500;
    Q->LFSR2 = 0x5500;
    Q->PanMask=0xff;

    Q->MuteMask=0;
    Q->SoloMask=0;

    strcpy(Q->SongMessage,"QuattroPlay (" __DATE__ " " __TIME__ ")");

    if(Q->BootSong && ((Q->TableOffset[Q_TOFFSET_SONGTABLE]&0xff)==0x0e))
        Q->SongRequest[0] = Q_TRACK_STATUS_START;

}

void Q_UpdateTick(Q_State *Q)
{
    Q->FrameCnt += 0x40;
    Q_UpdateTracks(Q);
    Q_UpdateVoices(Q);

    // C352_WriteFromStruct...
}

// LFSR random number generator... Generates same output as original. (verified with ncv2)
// source (sws2000): 0x56bc, 0x6e8c
uint16_t Q_GetRandom(uint16_t* lfsr)
{
    uint16_t d = *lfsr;
    d = (d<<1) | (((d>>14)^(d>>6))&1);
    *lfsr = d;
    return d;
}
