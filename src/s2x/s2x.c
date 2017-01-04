#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../qp.h"
#include "helper.h"
#include "track.h"
#include "voice.h"

void S2X_Init(S2X_State *S)
{
    S2X_LoopDetectionInit(S);
    //Q_GetMcuVer(S);
    S2X_Reset(S);
}

void S2X_Deinit(S2X_State *S)
{
    S2X_LoopDetectionFree(S);
}

void S2X_Reset(S2X_State *S)
{
    S2X_LoopDetectionReset(S);
    int i;

    memset(S->PCMChip.v,0,sizeof(S->PCMChip.v));
    memset(S->PCM,0,sizeof(S->PCM));
    memset(S->FM,0,sizeof(S->FM));

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

    S->FMBase = 0x4000;
    S->PCMBase = 0x10000;

    // some early games have PCM and FM swapped
    if(S2X_ReadWord(S,0x10000) == 0x0008)
    {
        S->PCMBase=0x4000;
        S->FMBase=0x10000;
    }

    S->FMLfo=0xff;

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
