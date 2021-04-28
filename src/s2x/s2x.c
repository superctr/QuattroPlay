#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../qp.h"

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"
#include "wsg.h"

#define SYSTEMNA (S->DriverType == S2X_TYPE_NA)
#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)

#define S1_WSG (S->DriverType == S2X_TYPE_SYSTEM1 || S->DriverType == S2X_TYPE_SYSTEM1_ALT)
#define S86_WSG (S->DriverType == S2X_TYPE_SYSTEM86)

void S2X_Init(S2X_State *S)
{
    QP_LoopDetect ld = {
        .TrackCnt = S2X_MAX_TRACKS,
        .DataSize = Game->DataSize,
        .SongCnt = 0x400,
        .CheckValid = S2X_LoopDetectValid,
        .Driver = S
    };
    S->LoopDetect = ld;
    if(QP_LoopDetectInit(&S->LoopDetect))
    {
        Q_DEBUG("Loop detection initialization failed\n");
    }
    S2X_InitDriverType(S);
    S2X_MakePitchTable(S);
    if(S1_WSG)
        S2X_S1WSGLoadWave(S);
    else if(S86_WSG)
        S2X_S86WSGLoadWave(S);
#if 0
    int i;
    for(i=0;i<128;i++)
        Q_DEBUG("%04x%s",S->PCMPitchTable[i],((i+1)&7) ? " " : "\n");
#endif
    S2X_Reset(S);
}

void S2X_Deinit(S2X_State *S)
{
    QP_LoopDetectFree(&S->LoopDetect);
}

void S2X_Reset(S2X_State *S)
{
    QP_LoopDetectReset(&S->LoopDetect);
    int i;

    memset(S->PCMChip.v,0,sizeof(S->PCMChip.v));
    memset(S->PCM,0,sizeof(S->PCM));
    memset(S->FM,0,sizeof(S->FM));
    memset(S->SE,0,sizeof(S->SE));

    for(i=0;i<S2X_MAX_VOICES;i++)
    {
        S2X_VoiceClear(S,i);
    }

    S->PCMChip.mute_mask=0;

    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        S->ParentSong[i] = S2X_MAX_TRACKS;
        //S->SongTimer[i] = 0;
    }

    memset(S->Track,0,sizeof(S->Track));
    memset(S->ActiveChannel,0,sizeof(S->ActiveChannel));
    memset(S->ChannelPriority,0,sizeof(S->ChannelPriority));

    S->FrameCnt=0;

    S->FMLfo=0xff;

    S->CJump=0;
    S->MuteMask=0;
    S->SoloMask=0;
}

void S2X_UpdateTick(S2X_State *S)
{
    S->FrameCnt ++;

    int i;
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        if(S->SongRequest[i] & S2X_TRACK_STATUS_START)
        {
            S2X_TrackInit(S,i);
        }
        else if(S->Track[i].Flags & S2X_TRACK_STATUS_BUSY)
        {
            S->Track[i].Flags = S->SongRequest[S->ParentSong[i]];
            S2X_TrackUpdate(S,i);
            S2X_TrackCalcVolume(S,i);
        }
    }
    for(i=0;i<S2X_MAX_VOICES;i++)
    {
        S2X_VoiceUpdate(S,i);
    }
    C352_write(&S->PCMChip,0x202,i); // update key-ons
}
