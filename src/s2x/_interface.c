#include <string.h>
#include <stdlib.h>

#include "../qp.h"
#include "../lib/vgm.h"
#include "helper.h"
#include "voice.h"

int S2X_IInit(union QP_Driver d,QP_Game *g)
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
    d.s2x->FMWriteTicks = 0;
    YM2151_init(&d.s2x->FMChip,d.s2x->FMClock);

    d.s2x->SoundRate = d.s2x->PCMChip.rate;
    d.s2x->FMDelta = d.s2x->FMChip.rate / d.s2x->SoundRate;
    d.s2x->FMWriteRate = 2.5;

    d.s2x->FMBase = 0;
    d.s2x->PCMBase = 0;

    config_t* cfg;

    d.s2x->ConfigFlags=0;
    int i,v;
    for(i=0;i<g->ConfigCount;i++)
    {
        cfg = &g->Config[i];
        v = atoi(cfg->data);
        if(!strcmp(cfg->name,"fm_volcalc") && v)
            d.s2x->ConfigFlags |= S2X_CFG_FM_VOL;
        else if(!strcmp(cfg->name,"pcm_adsr") && v>1)
            d.s2x->ConfigFlags |= S2X_CFG_PCM_NEWADSR;
        else if(!strcmp(cfg->name,"pcm_adsr") && v>0)
            d.s2x->ConfigFlags |= S2X_CFG_PCM_ADSR;
        else if(!strcmp(cfg->name,"pcm_paninvert") && v)
            d.s2x->ConfigFlags |= S2X_CFG_PCM_PAN;

        else if(!strcmp(cfg->name,"fm_writerate"))
            d.s2x->FMWriteRate = atof(cfg->data);
        else if(!strcmp(cfg->name,"fm_base"))
            d.s2x->FMBase = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"pcm_base"))
            d.s2x->PCMBase = strtol(cfg->data,NULL,0);
    }

    return 0;
}
void S2X_IDeinit(union QP_Driver d)
{
    S2X_Deinit(d.s2x);
}
void S2X_IVgmOpen(union QP_Driver d)
{
    vgm_datablock(0x92,0x1000000,d.s2x->PCMChip.wave,0x1000000,d.s2x->PCMChip.wave_mask,0);
    d.s2x->PCMChip.vgm_log = 1;

}
void S2X_IVgmClose(union QP_Driver d)
{
    d.s2x->PCMChip.vgm_log = 0;
    vgm_poke32(0xdc,d.s2x->PCMClock | Audio->state.MuteRear<<31);
    vgm_poke8(0xd6,288/4);

    vgm_poke32(0x30,d.s2x->FMClock);
}
void S2X_IReset(union QP_Driver d,QP_Game* g,int initial)
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

int S2X_IGetParamCnt(union QP_Driver d)
{
    return 0;
}
void S2X_ISetParam(union QP_Driver d,int id,int val)
{
    //d.quattro->Register[id&0xff] = val;
}
int S2X_IGetParam(union QP_Driver d,int id)
{
    return 0;
    //return d.quattro->Register[id&0xff];
}
int S2X_IGetParamName(union QP_Driver d,int id,char* buffer,int len)
{
    snprintf(buffer,len,"Register %02x",id);
    return -1;
}
char* S2X_IGetSongMessage(union QP_Driver d)
{
    return "System2x WIP Driver";
    //return d.quattro->SongMessage;
}
char* S2X_IGetDriverInfo(union QP_Driver d)
{
    return "System2x WIP Driver";
    //return (char*)Q_McuNames[d.quattro->McuType];
}
int S2X_IRequestSlotCnt(union QP_Driver d)
{
    return S2X_MAX_TRACKS; //Q_MAX_TRACKS;
}
int S2X_ISongCnt(union QP_Driver d,int slot)
{
    return 512; //d.quattro->SongCount;
}
void S2X_ISongRequest(union QP_Driver d,int slot,int val)
{
    d.s2x->SongRequest[slot&0x3f] = val | S2X_TRACK_STATUS_START;
}
void S2X_ISongStop(union QP_Driver d,int slot)
{
    d.s2x->SongRequest[slot&0x3f] &= 0x7ff;
}
void S2X_ISongFade(union QP_Driver d,int slot)
{
    d.s2x->SongRequest[slot&0x3f] |= S2X_TRACK_STATUS_FADE;
}
int S2X_ISongStatus(union QP_Driver d,int slot)
{
    return (d.s2x->SongRequest[slot&0x3f] & 0xf800);
}
int S2X_ISongId(union QP_Driver d,int slot)
{
    return d.s2x->SongRequest[slot&0x3f] & 0x7ff;
}
double S2X_ISongTime(union QP_Driver d,int slot)
{
    return d.s2x->SongTimer[slot&0x3f];
}

int S2X_IGetLoopCnt(union QP_Driver d,int slot)
{
    //return S2X_LoopDetectionGetCount(d.s2x,slot);
    return QP_LoopDetectGetCount(&d.s2x->LoopDetect,slot);
}
void S2X_IResetLoopCnt(union QP_Driver d)
{
    //S2X_LoopDetectionReset(d.s2x);
    QP_LoopDetectReset(&d.s2x->LoopDetect);
}

int S2X_IDetectSilence(union QP_Driver d)
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

double S2X_ITickRate(union QP_Driver d)
{
    return 120; // 120 Hz
}
void S2X_IUpdateTick(union QP_Driver d)
{
    S2X_UpdateTick(d.s2x);
}
double S2X_IChipRate(union QP_Driver d)
{
    return d.s2x->SoundRate;// d.quattro->Chip.rate;
    //return 85562;
}
void S2X_IUpdateChip(union QP_Driver d)
{
    S2X_State *S = d.s2x;
    S->FMTicks += S->FMDelta;
    S->FMWriteTicks += S->FMDelta;
    while(S->FMWriteTicks > S->FMWriteRate)
    {
        if((S->FMQueueRead&0x1ff) != (S->FMQueueWrite&0x1ff))
            S2X_OPMReadQueue(S);
        S->FMWriteTicks-=S->FMWriteRate;
    }
    while(S->FMTicks > 1.0)
    {
        YM2151_update(&d.s2x->FMChip);
        S->FMTicks-=1.0;
    }

    C352_update(&d.s2x->PCMChip);
}
void S2X_ISampleChip(union QP_Driver d,float* samples,int samplecnt)
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

uint32_t S2X_IGetMute(union QP_Driver d)
{
    return d.s2x->MuteMask;
}
void S2X_ISetMute(union QP_Driver d,uint32_t data)
{
    d.s2x->MuteMask = data;
    S2X_UpdateMuteMask(d.s2x);
}
uint32_t S2X_IGetSolo(union QP_Driver d)
{
    return d.s2x->SoloMask;
}
void S2X_ISetSolo(union QP_Driver d,uint32_t data)
{
    d.s2x->SoloMask = data;
    S2X_UpdateMuteMask(d.s2x);
}

void S2X_IDebugAction(union QP_Driver d,int id)
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
            printf("Track %02x: ID=%03x, start=%06x, current pos=%06x, loops=%d (%d/%04x/%d)\n",i,j,startpos,currpos,
                   QP_LoopDetectGetCount(&d.s2x->LoopDetect,i),
                   d.s2x->LoopDetect.Song[j].StackPos,
                   d.s2x->LoopDetect.Song[j].LoopId[d.s2x->LoopDetect.Song[j].StackPos],
                   d.s2x->LoopDetect.Song[j].LoopCnt);
                   //S2X_LoopDetectionGetCount(d.s2x,i),
                   //d.s2x->TrackLoopId[j],
                   //d.s2x->TrackLoopCount[j]);
            for(j=0;j<S2X_MAX_TRKCHN;j++)
            {
                if(T->Channel[j].Enabled)
                    printf("| Channel %d => Voice %02x\n",j,T->Channel[j].VoiceNo);
            }
        }
    }
}

int S2X_IGetVoiceCount(union QP_Driver d)
{
    return S2X_MAX_VOICES;
}

int S2X_IGetVoiceInfo(union QP_Driver d,int id,struct QP_DriverVoiceInfo *V)
{
    S2X_State* S = d.s2x;
    S2X_PCMVoice* PCM;
    S2X_FMVoice* FM;
    memset(V,0,sizeof(*V));

    int type = S2X_GetVoiceType(S,id);
    int index = S2X_GetVoiceIndex(S,id,type);
    switch(type)
    {
    default:
        return -1;
    case S2X_VOICE_TYPE_SE:
        V->Status=0;
        V->Preset=S->SEWave[index];
        V->VoiceType = VOICE_TYPE_PERCUSSION|VOICE_TYPE_PCM;
        if(S2X_C352_R(S,(index+16),C352_FLAGS) & (C352_FLG_BUSY|C352_FLG_KEYON))
            V->Status|=VOICE_STATUS_PLAYING;
        break;
    case S2X_VOICE_TYPE_FM:
        FM = &S->FM[index];
        V->Status=0;
        V->Track=0;
        if(FM->TrackNo)
        {
            V->Status|=VOICE_STATUS_ACTIVE;
            V->Track = FM->TrackNo-1;
        }
        if(FM->Flag&0x10)
            V->Status|=VOICE_STATUS_PLAYING;
        V->Channel = FM->ChannelNo;
        V->VoiceType = VOICE_TYPE_MELODY;
        V->PanType = PAN_TYPE_UNSIGNED;
        V->Pan = 0x80;
        V->VolumeMod = (S->ConfigFlags & S2X_CFG_FM_VOL) ? FM->Volume : (~FM->Volume)&0xff ;
        V->Volume = 0;
        V->Key = FM->Key+2;
        V->Pitch = FM->Pitch.Value+FM->Pitch.EnvMod+0x200;
        V->Preset = FM->InsNo;
        if(FM->Channel)
        {
            V->Volume = FM->Channel->Vars[S2X_CHN_VOL];
            V->Pitch += (FM->Channel->Vars[S2X_CHN_TRS]<<8)+FM->Channel->Vars[S2X_CHN_DTN];
            V->Key += FM->Channel->Vars[S2X_CHN_TRS];
            switch(FM->Channel->Vars[S2X_CHN_PAN]&0xc0)
            {
            default:
                V->PanType = PAN_TYPE_NONE;
                break;
            case 0x40:
                V->Pan = 0x00; // Left
                break;
            case 0x80:
                V->Pan = 0xff; // Right
                break;
            case 0xc0:
                V->Pan = 0x80; // Center
                break;
            }
        }
        break;
    case S2X_VOICE_TYPE_PCM:
        PCM = &S->PCM[index];
        // temporary until i add a key on flag
        V->Status = 0;
        V->Track = 0;
        if(PCM->TrackNo)
        {
            V->Status|=VOICE_STATUS_ACTIVE;
            V->Track = PCM->TrackNo-1;
        }
        if(PCM->Flag&0x10)
            V->Status|=VOICE_STATUS_PLAYING;
        V->Channel = PCM->ChannelNo;
        V->VoiceType = VOICE_TYPE_MELODY|VOICE_TYPE_PCM;
        V->PanType = PAN_TYPE_UNSIGNED;
        V->VolumeMod = PCM->Volume;
        V->Volume = 0;
        V->Key = PCM->Key-2;
        V->Pitch = PCM->Pitch.Value+PCM->Pitch.EnvMod-0x200;
        V->Preset = PCM->WaveNo;
        V->Pan = PCM->Pan;
        if(PCM->Channel)
        {
            V->Volume = PCM->Channel->Vars[S2X_CHN_VOL];
            V->Pitch += (PCM->Channel->Vars[S2X_CHN_TRS]<<8)+PCM->Channel->Vars[S2X_CHN_DTN];
            V->Key += PCM->Channel->Vars[S2X_CHN_TRS];
        }
        break;
    }
    return 0;
}

struct QP_DriverInterface S2X_CreateInterface()
{
    struct QP_DriverInterface d = {
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

        .IDebugAction = &S2X_IDebugAction,
        .IGetVoiceCount = &S2X_IGetVoiceCount,
        .IGetVoiceInfo = &S2X_IGetVoiceInfo,
    };
    return d;
}
