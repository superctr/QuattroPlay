#include <stdlib.h>
#include <string.h>

#include "../qp.h"
#include "helper.h"
#include "../lib/vgm.h"

uint8_t S2X_ReadByte(S2X_State *S,uint32_t d)
{
    return S->Data[d];
}

uint16_t S2X_ReadWord(S2X_State *S,uint32_t d)
{
    return ((S->Data[d]<<8) | (S->Data[d+1]<<0));
}

void S2X_UpdateMuteMask(S2X_State *S)
{
    if(S->SoloMask)
    {
        S->PCMChip.mute_mask = ~(S->SoloMask);
        S->FMChip.mute_mask = ~(S->SoloMask)>>24;
    }
    else
    {
        S->PCMChip.mute_mask = S->MuteMask;
        S->FMChip.mute_mask = S->MuteMask>>24;
    }
}

void S2X_OPMWrite(S2X_State *S,int ch,int op,int reg,uint8_t data)
{
    ch&=7;
    int fmreg = reg;
    if(reg >= 0x40)
        fmreg += (op*8)+ch;
    else if(reg >= 0x20)
        fmreg += ch;
    else if(reg == 0x08)
        data |= ch;

    if(S->PCMChip.vgm_log)
        vgm_write(0x54,0,fmreg,data);

    S2X_FMWrite w = {fmreg,data};

    //Q_DEBUG("write queue %02x (%02x %02x)\n",S->FMQueueWrite,fmreg,data);
    S->FMQueue[(S->FMQueueWrite++)&0x1ff] = w;

    // flush the queue!
    if((S->FMQueueWrite&0x1ff) == (S->FMQueueRead&0x1ff))
    {
        Q_DEBUG("flushing queue (OPM is not keeping up!)\n");
        do S2X_OPMReadQueue(S);
        while ((S->FMQueueWrite&0x1ff) != (S->FMQueueRead&0x1ff));
    }

    //printf(" -FM write %02x = %02x (Ch %d Op %d Reg %02x)\n",fmreg,data,ch,op,reg);
    //return YM2151_write_reg(&S->FMChip,fmreg,data);
    // there really should be a delay for a few frames to let the sound chip breathe....
    // i plan to implement an FM command queue, this should solve the problem of popping
    // when starting a new note
}

void S2X_OPMReadQueue(S2X_State *S)
{
    //Q_DEBUG("read  queue %02x (%02x %02x)\n",S->FMQueueRead,S->FMQueue[S->FMQueueRead].Reg,S->FMQueue[S->FMQueueRead].Data);
    S2X_FMWrite* w = &S->FMQueue[(S->FMQueueRead++)&0x1ff];
    return YM2151_write_reg(&S->FMChip,w->Reg,w->Data);
}

#ifndef Q_DISABLE_LOOP_DETECTION

void S2X_LoopDetectionInit(S2X_State *S)
{
    S->LoopCounterFlags = (uint32_t*)malloc(0x80000*sizeof(uint32_t));
    memset(S->LoopCounterFlags,0,0x80000*sizeof(uint32_t));
    S2X_LoopDetectionReset(S);
    S->NextLoopId = 1;
}
void S2X_LoopDetectionFree(S2X_State *S)
{
    free(S->LoopCounterFlags);
}
void S2X_LoopDetectionReset(S2X_State *S)
{
    uint32_t loopid = S->NextLoopId;
    S->NextLoopId = 0;
    memset(S->TrackLoopCount,0,sizeof(S->TrackLoopCount));
    memset(S->TrackLoopId,0,sizeof(S->TrackLoopId));
    S->NextLoopId = loopid+1;
}
void S2X_LoopDetectionCheck(S2X_State *S,int TrackNo,int stopped)
{
    if(S->NextLoopId == 0)
        return;

    uint16_t trackid = S->SongRequest[TrackNo]&0x7ff;
    uint32_t loopid = S->TrackLoopId[trackid];

    if(stopped)
        S->TrackLoopCount[trackid] = 255;

    if(loopid == 0)
    {
        loopid = S->NextLoopId++;
        S->TrackLoopId[trackid] = loopid;
    }

    uint32_t* data = &S->LoopCounterFlags[S->Track[TrackNo].Position+S->Track[TrackNo].PositionBase];
    if(*data == loopid && S->TrackLoopCount[trackid] < 100 &&
       S->Track[TrackNo].SubStackPos == 0 &&
       S->Track[TrackNo].RepeatStackPos == 0 &&
       S->Track[TrackNo].LoopStackPos == 0)
    {
        S->TrackLoopCount[trackid]++;
        loopid = S->NextLoopId++;
        S->TrackLoopId[trackid] = loopid;
        *data = loopid;
    }
    else if (*data != loopid)
    {
        *data = loopid;
    }
}

// disable loop detection for tracks that overlap
void S2X_LoopDetectionJumpCheck(S2X_State *S,int TrackNo)
{
    if(S->NextLoopId == 0)
        return;

    uint16_t trackid = S->SongRequest[TrackNo]&0x7ff;
    uint16_t trackid2;

    if(S->TrackLoopCount[trackid] > 100)
        return;

    uint32_t* data = &S->LoopCounterFlags[S->Track[TrackNo].Position+S->Track[TrackNo].PositionBase];

    int i;
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        trackid2 = S->SongRequest[i]&0x7ff;
        if(i != TrackNo && S->ParentSong[i] == S->ParentSong[TrackNo] && *data == S->TrackLoopId[trackid2] && *data != 0)
        {
            S->TrackLoopCount[trackid] = 254;
            return;
        }
    }
}

int8_t S2X_LoopDetectionGetCount(S2X_State *S,int TrackNo)
{
    int loopcount;
    uint8_t lowest = 255;
    int i;
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        if(S->ParentSong[i] == TrackNo)
        {
            loopcount = S->TrackLoopCount[S->SongRequest[i]&0x7ff];
            if(loopcount < lowest)
                lowest = loopcount;
        }
    }
    return (int8_t)lowest;
}
#endif
