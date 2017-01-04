#include <string.h>
#include <stdlib.h>

#include "../qp.h"
#include "../lib/vgm.h"
#include "helper.h"

int S2X_IInit(union _Driver d,game_t *g)
{
    memset(&d.s2x->PCMChip,0,sizeof(C352));
    memset(&d.s2x->FMChip,0,sizeof(YM2151));

    //d.quattro->McuType = Q_GetMcuTypeFromString(g->Type);
    d.s2x->PCMClock = 24576000; // temporary
    C352_init(&d.s2x->PCMChip,d.s2x->PCMClock);
    d.s2x->PCMChip.mulaw_type=C352_MULAW_TYPE_C140;
    d.s2x->PCMChip.vgm_log = 0;

    d.s2x->PCMChip.wave = g->WaveData;
    d.s2x->PCMChip.wave_mask = g->WaveMask;
    d.s2x->Data = g->Data;

    d.s2x->FMClock = 3579545;
    d.s2x->FMTicks = 0;
    YM2151_init(&d.s2x->FMChip,d.s2x->FMClock);

    d.s2x->SoundRate = d.s2x->PCMChip.rate;
    d.s2x->FMDelta = d.s2x->FMChip.rate / d.s2x->SoundRate;

    config_t* cfg;

    d.s2x->ConfigFlags=0;
    int i,v;
    for(i=0;i<g->ConfigCount;i++)
    {
        cfg = &g->Config[i];
        v = atoi(cfg->data);
        if(!strcmp(cfg->name,"fm_volcalc") && v)
            d.s2x->ConfigFlags |= S2X_CFG_FM_VOL;
        if(!strcmp(cfg->name,"pcm_adsr") && v>0)
            d.s2x->ConfigFlags |= S2X_CFG_PCM_ADSR;
        if(!strcmp(cfg->name,"pcm_adsr") && v>1)
            d.s2x->ConfigFlags |= S2X_CFG_PCM_NEWADSR;
        if(!strcmp(cfg->name,"pcm_paninvert") && v)
            d.s2x->ConfigFlags |= S2X_CFG_PCM_PAN;
    }

    return 0;
}
void S2X_IDeinit(union _Driver d)
{
    S2X_Deinit(d.s2x);
}
void S2X_IVgmOpen(union _Driver d)
{
    vgm_datablock(0x92,0x1000000,d.s2x->PCMChip.wave,0x1000000,d.s2x->PCMChip.wave_mask,0);
    d.s2x->PCMChip.vgm_log = 1;

}
void S2X_IVgmClose(union _Driver d)
{
    d.s2x->PCMChip.vgm_log = 0;
    vgm_poke32(0xdc,d.s2x->PCMClock | Audio->state.MuteRear<<31);
    vgm_poke8(0xd6,288/4);

    vgm_poke32(0x30,d.s2x->FMClock);
}
void S2X_IReset(union _Driver d,game_t* g,int initial)
{
    Q_DEBUG("S2X: Reset\n");
    YM2151_reset(&d.s2x->FMChip);

    d.s2x->FMQueueRead=0;
    d.s2x->FMQueueWrite=0;

    if(initial)
        S2X_Init(d.s2x);
    else
        S2X_Reset(d.s2x);
}

int S2X_IGetParamCnt(union _Driver d)
{
    return 0;
}
void S2X_ISetParam(union _Driver d,int id,int val)
{
    //d.quattro->Register[id&0xff] = val;
}
int S2X_IGetParam(union _Driver d,int id)
{
    return 0;
    //return d.quattro->Register[id&0xff];
}
int S2X_IGetParamName(union _Driver d,int id,char* buffer,int len)
{
    snprintf(buffer,len,"Register %02x",id);
    return -1;
}
char* S2X_IGetSongMessage(union _Driver d)
{
    return "System2x WIP Driver";
    //return d.quattro->SongMessage;
}
char* S2X_IGetDriverInfo(union _Driver d)
{
    return "System2x WIP Driver";
    //return (char*)Q_McuNames[d.quattro->McuType];
}
int S2X_IRequestSlotCnt(union _Driver d)
{
    return S2X_MAX_TRACKS; //Q_MAX_TRACKS;
}
int S2X_ISongCnt(union _Driver d,int slot)
{
    return 512; //d.quattro->SongCount;
}
void S2X_ISongRequest(union _Driver d,int slot,int val)
{
    d.s2x->SongRequest[slot&0x3f] = val | S2X_TRACK_STATUS_START;
}
void S2X_ISongStop(union _Driver d,int slot)
{
    d.s2x->SongRequest[slot&0x3f] &= 0x7ff;
}
void S2X_ISongFade(union _Driver d,int slot)
{
    d.s2x->SongRequest[slot&0x3f] |= S2X_TRACK_STATUS_FADE;
}
int S2X_ISongStatus(union _Driver d,int slot)
{
    return (d.s2x->SongRequest[slot&0x3f] & 0xf800);
}
int S2X_ISongId(union _Driver d,int slot)
{
    return d.s2x->SongRequest[slot&0x3f] & 0x7ff;
}
double S2X_ISongTime(union _Driver d,int slot)
{
    return d.s2x->SongTimer[slot&0x3f];
}

int S2X_IGetLoopCnt(union _Driver d,int slot)
{
    return S2X_LoopDetectionGetCount(d.s2x,slot);
}
void S2X_IResetLoopCnt(union _Driver d)
{
    S2X_LoopDetectionReset(d.s2x);
}

int S2X_IDetectSilence(union _Driver d)
{
    return 0;
#if 0
    int i, voicectr=0;
    for(i=0;i<Q_MAX_VOICES;i++)
        if(!d.quattro->Voice[i].Enabled)
            voicectr++;
    if(voicectr != Q_MAX_VOICES)
        return 1;
    return 0;
#endif
}

double S2X_ITickRate(union _Driver d)
{
    return 120; // 120 Hz
}
void S2X_IUpdateTick(union _Driver d)
{
    S2X_UpdateTick(d.s2x);
}
double S2X_IChipRate(union _Driver d)
{
    return d.s2x->SoundRate;// d.quattro->Chip.rate;
    //return 85562;
}
void S2X_IUpdateChip(union _Driver d)
{
    S2X_State *S = d.s2x;
    S->FMTicks += S->FMDelta;
    while(S->FMTicks > 1.0)
    {
        if((S->FMQueueRead&0x1ff) != (S->FMQueueWrite&0x1ff))
            S2X_OPMReadQueue(S);
        YM2151_update(&d.s2x->FMChip);
        S->FMTicks-=1.0;
    }

    C352_update(&d.s2x->PCMChip);
}
void S2X_ISampleChip(union _Driver d,float* samples,int samplecnt)
{
    int i;
    if(samplecnt > 4)
        samplecnt=4;
    for(i=0;i<samplecnt;i++)
        samples[i] = d.s2x->PCMChip.out[i] / 268435456;
    if(samplecnt > 2)
        samplecnt=2;
    for(i=0;i<samplecnt;i++)
    {
        double last = d.s2x->FMChip.out[i+2];
        double next = d.s2x->FMChip.out[i];
        samples[i] += (last+(d.s2x->FMTicks*(next-last)))/6;
        //samples[i] += (last+(d.s2x->FMTicks*(next-last)))/12; // for finallap
    }
}

uint32_t S2X_IGetMute(union _Driver d)
{
    return d.s2x->MuteMask;
}
void S2X_ISetMute(union _Driver d,uint32_t data)
{
    d.s2x->MuteMask = data;
    S2X_UpdateMuteMask(d.s2x);
}
uint32_t S2X_IGetSolo(union _Driver d)
{
    return d.s2x->SoloMask;
}
void S2X_ISetSolo(union _Driver d,uint32_t data)
{
    d.s2x->SoloMask = data;
    S2X_UpdateMuteMask(d.s2x);
}

void S2X_IDebugAction(union _Driver d,int id)
{
    int i,j;
    uint32_t startpos,currpos;
    S2X_Track *T;

    printf("Currently playing tracks:\n");
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        T = &d.s2x->Track[i];
        if(T->Flags & S2X_TRACK_STATUS_BUSY)
        {
            j=d.s2x->SongRequest[i]&0x1ff;
            startpos = T->PositionBase+S2X_ReadWord(d.s2x,T->PositionBase);
            startpos = T->PositionBase+S2X_ReadWord(d.s2x,startpos+(2*(j&0xff)));
            currpos = T->PositionBase+T->Position;
            printf("Track %02x: ID=%03x, start=%06x, current pos=%06x, loops=%d (%04x/%d)\n",i,j,startpos,currpos,
                   S2X_LoopDetectionGetCount(d.s2x,i),
                   d.s2x->TrackLoopId[j],
                   d.s2x->TrackLoopCount[j]);
            for(j=0;j<S2X_MAX_TRKCHN;j++)
            {
                if(T->Channel[j].Enabled)
                    printf("| Channel %d => Voice %02x\n",j,T->Channel[j].VoiceNo);
            }
        }
    }
}

struct _DriverInterface S2X_CreateInterface()
{
    struct _DriverInterface d = {
        .Name = "System 2x",

        .Type = DRIVER_SYSTEM2,

        // IInit is allowed to fail, this aborts the program
        .IInit = &S2X_IInit,

        .IDeinit = &S2X_IDeinit,
        .IVgmOpen = &S2X_IVgmOpen,
        .IVgmClose = &S2X_IVgmClose,
        .IReset = &S2X_IReset,

        .IGetParamCnt = &S2X_IGetParamCnt,
        .ISetParam = &S2X_ISetParam,
        .IGetParam = &S2X_IGetParam,

        .IGetParamName = &S2X_IGetParamName,
        .IGetSongMessage = &S2X_IGetSongMessage,
        .IGetDriverInfo = &S2X_IGetDriverInfo,

        .IRequestSlotCnt = &S2X_IRequestSlotCnt,
        .ISongCnt = &S2X_ISongCnt,
        .ISongRequest = &S2X_ISongRequest,
        .ISongStop = &S2X_ISongStop,
        .ISongFade = &S2X_ISongFade,
        .ISongStatus = &S2X_ISongStatus,
        .ISongId = &S2X_ISongId,
        .ISongTime = &S2X_ISongTime,

        .IGetLoopCnt = &S2X_IGetLoopCnt,
        .IResetLoopCnt = &S2X_IResetLoopCnt,

        .IDetectSilence = &S2X_IDetectSilence,

        .ITickRate = &S2X_ITickRate,
        .IUpdateTick = &S2X_IUpdateTick,
        .IChipRate = &S2X_IChipRate,
        .IUpdateChip = &S2X_IUpdateChip,
        .ISampleChip = &S2X_ISampleChip,

        .IGetMute = &S2X_IGetMute,
        .ISetMute = &S2X_ISetMute,
        .IGetSolo = &S2X_IGetSolo,
        .ISetSolo = &S2X_ISetSolo,

        .IDebugAction = &S2X_IDebugAction
    };
    return d;
}
