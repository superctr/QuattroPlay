/*
    Quattro - Helper functions, not found in the original sound driver...
*/
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"

#include "quattro.h"
#include "tables.h"
#include "helper.h"

#ifdef DEBUG
#define DISP_MCU_INFO 1
#endif

const char* Q_McuNames[Q_MCUTYPE_MAX] =
{
    "Unidentifed","C74/C75 (System22/NB)","C75 (NB-1/2, FL)","C76 (System11)","M37710 (SystemSuper22)","H8/3002 (ND-1)","H8/3002 (System12/23)",
};

const char* Q_NoteNames[12] =
{
    "A-","A#","B-","C-","C#","D-","D#","E-","F-","F#","G-","G#"
};

const char* Q_ConditionNames[16] =
{
    "==","!=",">=","<=",">","<","cc","cs",
    "pl","mi","XX","XX","XX","XX","XX","XX"
};

#ifdef DISP_MCU_INFO
const char MCUVerSearch_H8[] = "namco ltd.;System";
#endif

const Q_McuDriverVersion MCUVerSearch_Value[5] =
{
    Q_MCUVER_PRE,Q_MCUVER_Q00,Q_MCUVER_Q00,Q_MCUVER_Q01,Q_MCUVER_Q02
};

const char* MCUVerSearch_Quattro[5] =
{
    "Quattro","Q00","QX0","Q01","Q02"
};

// ************************************************************************* //

uint32_t Q_ReadPos(Q_State *Q, uint32_t d)
{
    return ((Q->McuData[d+2]<<16) | (Q->McuData[d+1]<<8) | (Q->McuData[d]<<0)) - Q->McuPosBase;
}

uint8_t Q_ReadByte(Q_State *Q,uint32_t d)
{
    return Q->McuData[d];
}

uint16_t Q_ReadWord(Q_State *Q,uint32_t d)
{
    return ((Q->McuData[d+1]<<8) | (Q->McuData[d]<<0));
}

uint16_t Q_ReadWordBE(Q_State *Q,uint32_t d)
{
    return ((Q->McuData[d]<<8) | (Q->McuData[d+1]<<0));
}

Q_McuType Q_GetMcuType(Q_State* Q)
{
    uint32_t d = (Q_ReadWordBE(Q,0)<<16)|Q_ReadWordBE(Q,2);

    if(d == 0x00550000)
        return Q_MCUTYPE_C74; // Also MCU_C75. Is there a way to tell the two apart?
    else if(d == 0x00001000)
        return Q_MCUTYPE_ND;
    else if(d == 0x00000100)
        return Q_MCUTYPE_S12; // Also MCU_S23.

    d = (Q_ReadWordBE(Q,0x100)<<16)|Q_ReadWordBE(Q,0x102);

    if(d == 0x00550000)
        return Q_MCUTYPE_C76;

    d = (Q_ReadWordBE(Q,+0xc000)<<16)|Q_ReadWordBE(Q,0xc002);

    if(d == 0x5332322d) // first part of 'S22-BIOS'
        return Q_MCUTYPE_SS22;

    return Q_MCUTYPE_UNIDENTIFIED;
}

void Q_GetMcuVer(Q_State* Q)
{
    int i;
    int l;

    Q->McuType = Q_GetMcuType(Q);
    Q->McuVer = Q_MCUVER_PRE;

    Q_GetOffsets(Q);

    Q_DEBUG("Detected MCU Type: %s\n",Q_McuNames[Q->McuType]);

#ifdef DISP_MCU_INFO
    static char temp[17];
    // attempt to find H8 program version
    for(i=Q->McuDrvStartPos;i<Q->McuDrvEndPos;i++)
    {
        if(memcmp(Q->McuData+i,MCUVerSearch_H8,strlen(MCUVerSearch_H8)) == 0)
        {
            uint32_t l = strlen((char*)Q->McuData+i)+2;
            printf("MCU Version: %s.%s\nDate code (Raw): 0x%04x%04x 0x%04x%04x\n",Q->McuData+i,Q->McuData+i+l,
                   Q_ReadWordBE(Q,i-8),Q_ReadWordBE(Q,i-6),Q_ReadWordBE(Q,i-4),Q_ReadWordBE(Q,i-2));
            printf("Date code: %04x-%02x-%02x %02x:%02x:%02x\n",
                   Q_ReadWordBE(Q,i-8),*(Q->McuData+i-6),*(Q->McuData+i-5),*(Q->McuData+i-3),*(Q->McuData+i-2),*(Q->McuData+i-1));
            break;
        }
    }

    if(Q->McuType == Q_MCUTYPE_SS22)
    {
        memcpy(temp,Q->McuData+Q->McuDrvStartPos,0x10);
        printf("MCU version: %s ",temp);
        memcpy(temp,Q->McuData+Q->McuDrvStartPos+0x10,0x10);
        printf("%s ",temp);
        memcpy(temp,Q->McuData+Q->McuDrvStartPos+0x20,0x10);
        printf("%s\n",temp);
    }
#endif

    // attempt to find sound driver string
    for(i=Q->McuDrvStartPos;i<Q->McuDrvEndPos;i++)
    {
        for(l=0;l<5;l++)
        {
            if(memcmp(Q->McuData+i,MCUVerSearch_Quattro[l],strlen(MCUVerSearch_Quattro[l])) == 0)
            {
                Q->McuVer = MCUVerSearch_Value[l];
                Q_DEBUG("Sound driver: %s\n",Q->McuData+i);
                goto skip;
            }
        }
    }
skip:

    // Chip clock
    switch(Q->McuType)
    {
    default:
        Q->ChipClock = 24576000;
        break;
    case Q_MCUTYPE_C76:
    case Q_MCUTYPE_S12:
    //case Q_MCUTYPE_S23:
        Q->ChipClock = 25401600;
        break;
    }

    if(!Q->Chip.rate)
        Q->Chip.rate = Q->ChipClock/288;

    for(i=0;i<Q_TOFFSET_MAX;i++)
        Q->TableOffset[i] = Q_ReadWord(Q,Q->McuHeaderPos+(i*2))+Q->McuDataPosBase;

    Q->SongCount = (Q->TableOffset[Q_TOFFSET_WAVETABLE]-Q->TableOffset[Q_TOFFSET_SONGTABLE])/3;

    // filter out illegal entries at end
    while(Q_GetSongPos(Q,Q->SongCount-1) < 0)
        Q->SongCount--;

    Q_DEBUG("Song count: %02x\n",Q->SongCount);

}


void Q_GetOffsets(Q_State *Q)
{
    Q->McuHeaderPos = 0x10000;
    Q->McuPosBase = 0x200000;
    switch(Q->McuType)
    {
    case Q_MCUTYPE_C74:
    case Q_MCUTYPE_C75:
    case Q_MCUTYPE_C76:
    case Q_MCUTYPE_UNIDENTIFIED:
    default:
        Q->McuDrvStartPos=0;
        Q->McuDrvEndPos=0x10000;
        break;
    case Q_MCUTYPE_SS22:
        Q->McuDrvStartPos=0xc000;
        Q->McuDrvEndPos=0x10000;
        break;
    case Q_MCUTYPE_ND:
    case Q_MCUTYPE_S12:
    //case Q_MCUTYPE_S23:
        Q->McuDrvStartPos=0;
        Q->McuDrvEndPos=0x8000;
        Q->McuPosBase=0;
        Q->McuHeaderPos=0x8000;

        // raceon
        if(Q_ReadWord(Q,Q->McuHeaderPos) == 0xffff)
            Q->McuHeaderPos+=0x8000;
        break;
    }
    Q->McuDataPosBase = Q->McuHeaderPos&0xff0000;
}

int32_t Q_GetSongPos(Q_State *Q,uint16_t id)
{
    uint32_t d;
    uint32_t pos;

    if(id >= Q->SongCount)
        return -1;
    d = Q->TableOffset[Q_TOFFSET_SONGTABLE] + (id*3);
    pos = ((Q->McuData[d+2]<<16) | (Q->McuData[d+1]<<8) | (Q->McuData[d]<<0));
    if(pos == 0)
        return -1;
    pos -= Q->McuPosBase;
    if(pos > 0x80000)
        return -1;

    return pos;
}

// ===

uint8_t Q_ReadTrackInfo(Q_State *Q, int trk, int index)
{
    if(index > 0x28)
        return 0;
    else if(index > 0x25) // 26-28
        return Q->Track[trk].LoopCount[index-0x26];
    else if(index > 0x21) // 22-25
        return Q->Track[trk].RepeatCount[index-0x22];
    return *(uint8_t*)((void*)&Q->Track[trk]+Q_TrackStructMap[index]);
}
uint8_t Q_ReadChannelInfo(Q_State *Q, int trk, int ch, int index)
{
    return *(uint8_t*)((void*)&Q->Track[trk].Channel[ch]+Q_ChannelStructMap[index]);
}

void Q_WriteTrackInfo(Q_State *Q, int trk, int index, uint8_t data)
{
    *(uint8_t*)((void*)&Q->Track[trk]+Q_TrackStructMap[index]) = data;
}
void Q_WriteChannelInfo(Q_State *Q, int trk, int ch, int index, uint8_t data)
{
    *(uint8_t*)((void*)&Q->Track[trk].Channel[ch]+Q_ChannelStructMap[index]) = data;
}
void Q_WriteMacroInfo(Q_State *Q, int macro, int index, uint8_t data)
{
    *(uint8_t*)((void*)&Q->ChannelMacro[macro]+Q_ChannelStructMap[index]) = data;
}
void Q_UpdateMuteMask(Q_State *Q)
{
    if(Q->SoloMask)
        Q->Chip.mute_mask = ~(Q->SoloMask);
    else
        Q->Chip.mute_mask = Q->MuteMask;
}

#ifndef Q_DISABLE_LOOP_DETECTION
void Q_LoopDetectionInit(Q_State *Q)
{
    Q->LoopCounterFlags = (uint32_t*)malloc(0x80000*sizeof(uint32_t));
    memset(Q->LoopCounterFlags,0,0x80000*sizeof(uint32_t));
    Q_LoopDetectionReset(Q);
    Q->NextLoopId = 1;
}
void Q_LoopDetectionFree(Q_State *Q)
{
    free(Q->LoopCounterFlags);
}
void Q_LoopDetectionReset(Q_State *Q)
{
    uint32_t loopid = Q->NextLoopId;
    Q->NextLoopId = 0;
    memset(Q->TrackLoopCount,0,sizeof(Q->TrackLoopCount));
    memset(Q->TrackLoopId,0,sizeof(Q->TrackLoopId));
    Q->NextLoopId = loopid+1;
}
void Q_LoopDetectionCheck(Q_State *Q,int TrackNo,int stopped)
{
    if(Q->NextLoopId == 0)
        return;

    uint16_t trackid = Q->SongRequest[TrackNo]&0x7ff;
    uint32_t loopid = Q->TrackLoopId[trackid];

    if(stopped)
        Q->TrackLoopCount[trackid] = 255;

    if(loopid == 0)
    {
        loopid = Q->NextLoopId++;
        Q->TrackLoopId[trackid] = loopid;
    }

    uint32_t* data = &Q->LoopCounterFlags[Q->Track[TrackNo].Position];
    if(*data == loopid && Q->TrackLoopCount[trackid] < 100 &&
       Q->Track[TrackNo].SubStackPos == 0 &&
       Q->Track[TrackNo].RepeatStackPos == 0 &&
       Q->Track[TrackNo].LoopStackPos == 0)
    {
        Q->TrackLoopCount[trackid]++;
        loopid = Q->NextLoopId++;
        Q->TrackLoopId[trackid] = loopid;
        *data = loopid;
    }
    else if (*data != loopid)
    {
        *data = loopid;
    }
}

// disable loop detection for tracks that overlap
void Q_LoopDetectionJumpCheck(Q_State *Q,int TrackNo)
{
    if(Q->NextLoopId == 0)
        return;

    uint16_t trackid = Q->SongRequest[TrackNo]&0x7ff;
    uint16_t trackid2;
    //uint32_t loopid = Q->TrackLoopId[trackid];

    if(Q->TrackLoopCount[trackid] > 100)
        return;

    uint32_t* data = &Q->LoopCounterFlags[Q->Track[TrackNo].Position];

    int i;
    for(i=0;i<Q_MAX_TRACKS;i++)
    {
        trackid2 = Q->SongRequest[i]&0x7ff;
        if(i != TrackNo && Q->ParentSong[i] == Q->ParentSong[TrackNo] && *data == Q->TrackLoopId[trackid2] && *data != 0)
        {
            Q->TrackLoopCount[trackid] = 254;
            return;
        }
    }
}

int8_t Q_LoopDetectionGetCount(Q_State *Q,int TrackNo)
{
    int loopcount;
    uint8_t lowest = 255;
    int i;
    for(i=0;i<Q->TrackCount;i++)
    {
        if(Q->ParentSong[i] == TrackNo)
        {
            loopcount = Q->TrackLoopCount[Q->SongRequest[i]&0x7ff];
            if(loopcount < lowest)
                lowest = loopcount;
        }
    }
    return (int8_t)lowest;
}

#endif // Q_DISABLE_LOOP_DETECTION

