#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../qp.h"

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"

#define SYSTEMNA (S->DriverType == S2X_TYPE_NA)
#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)

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
    S2X_MakePitchTable(S);
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

/*
    for(i=0;i<S2X_MAX_VOICES_PCM;i++)
    {
        S2X_C352_W(S,i,C352_FLAGS,0);
    }
*/
    S->PCMChip.mute_mask=0;

    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        S->ParentSong[i] = S2X_MAX_TRACKS;
        //S->SongTimer[i] = 0;
    }

    memset(S->Track,0,sizeof(S->Track));
    memset(S->ActiveChannel,0,sizeof(S->ActiveChannel));
    memset(S->ChannelPriority,0,sizeof(S->ChannelPriority));

    //Q->BaseFadeout=0x9a;        // add this to config ini?
    //Q->BaseAttenuation=0x1c;

    S->FrameCnt=0;

    if(SYSTEM1)
    {
        if(!S->FMBase)
            S->FMBase = 0x10000;
        S->PCMBase=S->FMBase;
    }
    else if(SYSTEMNA)
    {
        S->SongCount[0] = S2X_ReadByte(S,S->PCMBase+0x11);
        S->SongCount[1] = S2X_ReadByte(S,S->PCMBase+0x10011);
        Q_DEBUG("base = %06x\nmax(1) = %02x\nmax(2) = %02x\n",S->PCMBase,S->SongCount[0],S->SongCount[1]);
    }
    else
    {
        if(!S->FMBase)
        {
            S->FMBase = 0x4000;
            // some early games have PCM and FM swapped
            if(S2X_ReadWord(S,0x10000) == 0x0008)
                S->FMBase=0x10000;
        }
        if(!S->PCMBase)
        {
            S->PCMBase = 0x10000;
            // some early games have PCM and FM swapped
            if(S2X_ReadWord(S,0x10000) == 0x0008)
                S->PCMBase=0x4000;
        }
    }

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
