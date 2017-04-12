#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../qp.h"
#include "../lib/vgm.h"

#include "s2x.h"
#include "helper.h"
#include "tables.h"
#include "voice.h"
#include "wsg.h"

#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)
#define SYSTEMNA (S->DriverType == S2X_TYPE_NA)

int S2X_IInit(void* d,QP_Game *g)
{
    S2X_State* S = d;

    S2X_ReadConfig(S,g);

    memset(&S->PCMChip,0,sizeof(C352));
    memset(&S->FMChip,0,sizeof(YM2151));

    S->PCMClock = SYSTEMNA ? 50113000/2 : 49152000/2; // sound chip freq is master clock / 2
    C352_init(&S->PCMChip,S->PCMClock);
    S->PCMChip.vgm_log = 0;
    if(SYSTEMNA)
    {
        S->PCMChip.wave = g->Data;
        S->PCMChip.wave_mask = 0x7fffff; // TODO: have a proper address mask...
    }
    else
    {
        S->PCMChip.mulaw_type=C352_MULAW_TYPE_C140;
        S->PCMChip.wave = g->WaveData;
        S->PCMChip.wave_mask = g->WaveMask;
    }
    S->Data = g->Data;

    S->FMClock = 3579545;
    S->FMTicks = 0;
    S->FMWriteTicks = 0;
    YM2151_init(&S->FMChip,S->FMClock);

    S->SoundRate = S->PCMChip.rate;
    S->FMDelta = S->FMChip.rate / S->SoundRate;
    S->FMWriteRate = SYSTEM1 ? 1.0 : 2.5;

    g->MuteRear = 1;

    return 0;
}
void S2X_IDeinit(void* d)
{
    S2X_State* S = d;
    S2X_Deinit(S);
}
void S2X_IVgmOpen(void* d)
{
    S2X_State* S = d;

    if(SYSTEM1)
    {
        S2X_InitDriverType(S);
        S2X_WSGLoadWave(S);
    }

    vgm_datablock(0x92,0x1000000,S->PCMChip.wave,0x1000000,S->PCMChip.wave_mask,0);
    S->PCMChip.vgm_log = 1;
}
void S2X_IVgmClose(void* d)
{
    S2X_State* S = d;
    S->PCMChip.vgm_log = 0;
    vgm_poke32(0xdc,S->PCMClock | Audio->state.MuteRear<<31);
    vgm_poke8(0xd6,288/4);

    vgm_poke32(0x30,S->FMClock);
}
void S2X_IReset(void* d,QP_Game* g,int initial)
{
    S2X_State* S = d;
    Q_DEBUG("S2X: Reset\n");
    YM2151_reset(&S->FMChip);

    S->FMQueueRead=0;
    S->FMQueueWrite=0;

    if(initial)
        S2X_Init(S);
    else
        S2X_Reset(S);
}

// ============================================================================

int S2X_IGetParamCnt(void* d)
{
    S2X_State* S = d;
    if(SYSTEMNA)
        return 2;
    return 1;
}
void S2X_ISetParam(void* d,int id,int val)
{
    S2X_State* S = d;
    switch(id)
    {
    case 0:
        S->CJump = val>0;
        break;
    case 1:
        S->BankSelect = val%S2X_MAX_BANK;
        break;
    default:
        return;
    }
}
int S2X_IGetParam(void* d,int id)
{
    S2X_State* S = d;
    switch(id)
    {
    case 0:
        return S->CJump;
    case 1:
        return S->BankSelect;
    default:
        return 0;
    }
}
int S2X_IGetParamName(void* d,int id,char* buffer,int len)
{
    switch(id)
    {
    case 0:
        strncpy(buffer,"Jump Condition",len);
        break;
    case 1:
        strncpy(buffer,"Bank Select",len);
        break;
    default:
        return 0;
    }
    return -1;
}
char* S2X_IGetSongMessage(void* d)
{
    S2X_State*S = d;
    if(S->BankName[S->BankSelect])
        return S->BankName[S->BankSelect];
    return "System2x WIP Driver";
}
char* S2X_IGetDriverInfo(void* d)
{
    S2X_State*S = d;
    return S2X_DriverTypes[S->DriverType];
}
int S2X_IRequestSlotCnt(void* d)
{
    return S2X_MAX_TRACKS;
}
int S2X_ISongCnt(void* d,int slot)
{
    return 512;
}
void S2X_ISongRequest(void* d,int slot,int val)
{
    S2X_State* S = d;
    S->SongRequest[slot&0x3f] = val | S2X_TRACK_STATUS_START;
}
void S2X_ISongStop(void* d,int slot)
{
    S2X_State* S = d;
    S->SongRequest[slot&0x3f] &= 0x7ff;
}
void S2X_ISongFade(void* d,int slot)
{
    S2X_State* S = d;
    S->SongRequest[slot&0x3f] |= S2X_TRACK_STATUS_FADE;
}
int S2X_ISongStatus(void* d,int slot)
{
    S2X_State* S = d;
    if(!(S->SongRequest[slot&0x3f]&0xf800) && (S->Track[slot&0x3f].Flags & 0xf800))
        return SONG_STATUS_STOPPING;
    return (S->SongRequest[slot&0x3f] & 0xf800);
}
int S2X_ISongId(void* d,int slot)
{
    S2X_State* S = d;
    return S->SongRequest[slot&0x3f] & 0x7ff;
}
double S2X_ISongTime(void* d,int slot)
{
    S2X_State* S = d;
    return S->SongTimer[slot&0x3f];
}

int S2X_IGetLoopCnt(void* d,int slot)
{
    S2X_State* S = d;
    return QP_LoopDetectGetCount(&S->LoopDetect,slot);
}
void S2X_IResetLoopCnt(void* d)
{
    S2X_State* S = d;
    QP_LoopDetectReset(&S->LoopDetect);
}

int S2X_IDetectSilence(void* d)
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

double S2X_ITickRate(void* d)
{
    S2X_State *S = d;
    if(SYSTEM1)
        return 60;
    else
        return 120; // 120 Hz
}
void S2X_IUpdateTick(void* d)
{
    S2X_UpdateTick(d);
}
double S2X_IChipRate(void* d)
{
    S2X_State* S = d;
    return S->SoundRate;
}
void S2X_IUpdateChip(void* d)
{
    S2X_State *S = d;
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
        YM2151_update(&S->FMChip);
        S->FMTicks-=1.0;
    }

    C352_update(&S->PCMChip);
}
void S2X_ISampleChip(void* d,float* samples,int samplecnt)
{
    S2X_State* S = d;
    int i;
    if(samplecnt > 4)
        samplecnt=4;
    for(i=0;i<samplecnt;i++)
        samples[i] = S->PCMChip.out[i] / (1<<28);
    if(samplecnt > 2)
        samplecnt=2;
    for(i=0;i<samplecnt;i++)
    {
        double last = S->FMChip.out[i+2];
        double next = S->FMChip.out[i];
        samples[i] += (last+(S->FMTicks*(next-last)))/6;
        //samples[i] += (last+(S->FMTicks*(next-last)))/12; // for finallap
    }
}

uint32_t S2X_IGetMute(void* d)
{
    S2X_State* S = d;
    return S->MuteMask;
}
void S2X_ISetMute(void* d,uint32_t data)
{
    S2X_State* S = d;
    S->MuteMask = data;
    S2X_UpdateMuteMask(S);
}
uint32_t S2X_IGetSolo(void* d)
{
    S2X_State* S = d;
    return S->SoloMask;
}
void S2X_ISetSolo(void* d,uint32_t data)
{
    S2X_State* S = d;
    S->SoloMask = data;
    S2X_UpdateMuteMask(S);
}

void S2X_IDebugAction(void* d,int id)
{
    S2X_State* S = d;
    int i,j,ldsng;
    uint32_t startpos,currpos;
    S2X_Track *T;

    printf("Currently playing tracks:\n");
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        T = &S->Track[i];
        if(T->Flags & S2X_TRACK_STATUS_BUSY)
        {
            j=S->SongRequest[i]&0x1ff;
            ldsng=S->LoopDetect.Track[i].SongId;
            startpos = T->PositionBase+S2X_ReadWord(S,T->PositionBase);
            startpos = T->PositionBase+S2X_ReadWord(S,startpos+(2*(j&0xff)));
            currpos = T->PositionBase+T->Position;
            printf("Track %02x: ID=%03x, start=%06x, current pos=%06x, loops=%d (%d/%04x/%d)\n",i,j,startpos,currpos,
                   QP_LoopDetectGetCount(&S->LoopDetect,i),
                   S->LoopDetect.Song[ldsng].StackPos,
                   S->LoopDetect.Song[ldsng].LoopId[S->LoopDetect.Song[ldsng].StackPos],
                   S->LoopDetect.Song[ldsng].LoopCnt);
            for(j=0;j<S2X_MAX_TRKCHN;j++)
            {
                if(T->Channel[j].Enabled && SYSTEMNA)
                    printf("| Channel %d => Voice %02x (Bank=%02x)\n",j,T->Channel[j].VoiceNo,S->WaveBank[T->Channel[j].VoiceNo/4]);
                else if(T->Channel[j].Enabled)
                    printf("| Channel %d => Voice %02x\n",j,T->Channel[j].VoiceNo);
            }
        }
    }
}

int S2X_IGetVoiceCount(void* d)
{
    return S2X_MAX_VOICES;
}

int S2X_IGetVoiceInfo(void* d,int id,struct QP_DriverVoiceInfo *V)
{
    S2X_State* S = d;
    S2X_PCMVoice* PCM;
    S2X_WSGVoice* WSG;
    S2X_FMVoice* FM;
    memset(V,0,sizeof(*V));

    int index = S->Voice[id].Index;
    switch(S->Voice[id].Type)
    {
    default:
        return -1;
    case S2X_VOICE_TYPE_SE:
        V->Status=0;
        V->Preset=S->SE[index].Wave;
        V->VoiceType = VOICE_TYPE_PERCUSSION|VOICE_TYPE_PCM;
        if(S->SE[index].Type && S2X_C352_R(S,(S->SE[index].Voice),C352_FLAGS) & (C352_FLG_BUSY|C352_FLG_KEYON))
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
        V->VolumeMod = (S->ConfigFlags & S2X_CFG_FM_VOL) ? FM->Volume : ((~FM->Volume)&0x7f)<<1 ;
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
    case S2X_VOICE_TYPE_WSG:
        WSG = &S->WSG[index];
        V->Status = 0;
        V->Track = 0;
        if(WSG->TrackNo)
        {
            V->Status|=VOICE_STATUS_ACTIVE;
            V->Track = WSG->TrackNo-1;
        }
        if(WSG->Channel)
        {
            double afreq = (440*32)*(65536/(double)S->PCMChip.rate);
            double keyfreq = WSG->Channel->WSG.Freq * (72.0/256.0);
            double key = 46 + (12.0*log2(keyfreq/afreq));

            V->Pitch = 256.0*key;
            V->Key = key+0.5;

            V->Preset = WSG->Channel->WSG.WaveNo;
            if(WSG->Channel->WSG.Env[0].Val || WSG->Channel->WSG.Env[1].Val)
                V->Status|=VOICE_STATUS_PLAYING;
        }

        V->Channel = WSG->ChannelNo;
        V->VoiceType = VOICE_TYPE_MELODY|VOICE_TYPE_PCM;
    }
    return 0;
}
uint16_t S2X_IGetVoiceStatus(void* d,int id)
{
    S2X_State* S = d;
    if(id>=S2X_MAX_VOICES)
        return 0;
    uint16_t v=0;
    int index = S->Voice[id].Index;
    switch(S->Voice[id].Type)
    {
    case S2X_VOICE_TYPE_FM:
        if(S->FM[index].TrackNo)
            v = 0x8000|(S->FM[index].TrackNo-1)<<8|(S->FM[index].ChannelNo);
        if(S->FM[index].Flag&0x10) // flag 0x80 is almost always set for FM tracks
            v |= 0x80;
        break;
    case S2X_VOICE_TYPE_PCM:
        if(S->PCM[index].TrackNo)
            v = 0x8000|(S->PCM[index].TrackNo-1)<<8|(S->PCM[index].ChannelNo);
        if(S->PCM[index].Flag&0x80)
            v |= 0x80;
        // for NA-1/NA-2
        else if(S->SE[index&7].Type && S->SE[index&7].Voice == index && S2X_C352_R(S,index,C352_FLAGS) & (C352_FLG_BUSY|C352_FLG_KEYON))
            v = 0xf000 | S->SE[index&7].Wave;
        break;
    case S2X_VOICE_TYPE_SE:
        if(S->SE[index].Type && S2X_C352_R(S,(16+index),C352_FLAGS) & (C352_FLG_BUSY|C352_FLG_KEYON))
            v = 0xf000 | S->SE[index].Wave;
        break;
    case S2X_VOICE_TYPE_WSG:
        if(S->WSG[index].TrackNo)
            v = 0x8000|(S->WSG[index].TrackNo-1)<<8|(S->WSG[index].ChannelNo);
        break;
    default:
        return 0;
    }
    return v;
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
        .IGetVoiceStatus = &S2X_IGetVoiceStatus,
    };
    return d;
}
