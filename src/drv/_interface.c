#include <string.h>

#include "../qp.h"
#include "../lib/vgm.h"

#include "quattro.h"
#include "helper.h"

int Q_IInit(void* d,QP_Game *g)
{
    Q_State *Q = d;
    memset(&Q->Chip,0,sizeof(C352));

    Q->McuType = Q_GetMcuTypeFromString(g->Type);
    Q->ChipClock = g->ChipFreq;
    C352_init(&Q->Chip,g->ChipFreq);
    Q->Chip.mulaw_type = C352_MULAW_TYPE_C352;
    Q->Chip.vgm_log = 0;

    Q->Chip.wave = g->WaveData;
    Q->Chip.wave_mask = g->WaveMask;
    Q->McuData = g->Data;
    return 0;
}
void Q_IDeinit(void* d)
{
    Q_Deinit(d);
}
void Q_IVgmOpen(void* d)
{
    Q_State *Q = d;
    vgm_datablock(0x92,0x1000000,Q->Chip.wave,0x1000000,Q->Chip.wave_mask,0);
    Q->Chip.vgm_log = 1;
}
void Q_IVgmClose(void* d)
{
    Q_State *Q = d;
    Q->Chip.vgm_log = 0;
    vgm_poke32(0xdc,Q->ChipClock | Audio->state.MuteRear<<31);
    vgm_poke8(0xd6,288/4);
}
void Q_IReset(void* d,QP_Game* g,int initial)
{
    Q_State *Q = d;
    Q->PortaFix=g->PortaFix;
    Q->BootSong=g->BootSong;

    if(initial)
        Q_Init(Q);
    else
        Q_Reset(Q);

    if(initial && g->AutoPlay >= 0)
        Q->BootSong=2;
}

// ============================================================================

int Q_IGetParamCnt(void* d)
{
    return 256;
}
void Q_ISetParam(void* d,int id,int val)
{
    Q_State *Q = d;
    Q->Register[id&0xff] = val;
}
int Q_IGetParam(void* d,int id)
{
    Q_State *Q = d;
    return Q->Register[id&0xff];
}
int Q_IGetParamName(void* d,int id,char* buffer,int len)
{
    snprintf(buffer,len,"Register %02x",id);
    return -1;
}
char* Q_IGetSongMessage(void* d)
{
    Q_State *Q = d;
    return Q->SongMessage;
}
char* Q_IGetDriverInfo(void* d)
{
    Q_State *Q = d;
    return (char*)Q_McuNames[Q->McuType];
}
int Q_IRequestSlotCnt(void* d)
{
    return Q_MAX_TRACKS;
}
int Q_ISongCnt(void* d,int slot)
{
    Q_State *Q = d;
    return Q->SongCount;
}
void Q_ISongRequest(void* d,int slot,int val)
{
    Q_State *Q = d;
    Q->SongRequest[slot&0x3f] = val | Q_TRACK_STATUS_START;
}
void Q_ISongStop(void* d,int slot)
{
    Q_State *Q = d;
    Q->SongRequest[slot&0x3f] &= 0x7ff;
}
void Q_ISongFade(void* d,int slot)
{
    Q_State *Q = d;
    Q->SongRequest[slot&0x3f] |= Q_TRACK_STATUS_FADE;
}
int Q_ISongStatus(void* d,int slot)
{
    Q_State *Q = d;
    if(!(Q->SongRequest[slot&0x3f]&0xf800) && (Q->Track[slot&0x3f].Flags & 0xf800))
        return SONG_STATUS_STOPPING;
    return (Q->SongRequest[slot&0x3f] & 0xf800);
}
int Q_ISongId(void* d,int slot)
{
    Q_State *Q = d;
    return Q->SongRequest[slot&0x3f] & 0x7ff;
}
double Q_ISongTime(void* d,int slot)
{
    Q_State *Q = d;
    return Q->SongTimer[slot&0x3f];
}

int Q_IGetLoopCnt(void* d,int slot)
{
    Q_State *Q = d;
    return Q_LoopDetectionGetCount(Q,slot);
}
void Q_IResetLoopCnt(void* d)
{
    Q_State *Q = d;
    Q_LoopDetectionReset(Q);
}

int Q_IDetectSilence(void* d)
{
    Q_State *Q = d;
    int i, voicectr=0;
    for(i=0;i<Q_MAX_VOICES;i++)
        if(!Q->Voice[i].Enabled)
            voicectr++;
    if(voicectr != Q_MAX_VOICES)
        return 1;
    return 0;
}

double Q_ITickRate(void* d)
{
    return 120; // 120 Hz
}
void Q_IUpdateTick(void* d)
{
    Q_UpdateTick(d);
}
double Q_IChipRate(void* d)
{
    Q_State *Q = d;
    return Q->Chip.rate;
}
void Q_IUpdateChip(void* d)
{
    Q_State *Q = d;
    C352_update(&Q->Chip);
}
void Q_ISampleChip(void* d,float* samples,int samplecnt)
{
    Q_State *Q = d;
    int i;
    if(samplecnt > 4)
        samplecnt=4;
    for(i=0;i<samplecnt;i++)
        samples[i] = Q->Chip.out[i] / (1<<28);
}

uint32_t Q_IGetMute(void* d)
{
    Q_State *Q = d;
    return Q->MuteMask;
}
void Q_ISetMute(void* d,uint32_t data)
{
    Q_State *Q = d;
    Q->MuteMask = data;
    Q_UpdateMuteMask(Q);
}
uint32_t Q_IGetSolo(void* d)
{
    Q_State *Q = d;
    return Q->SoloMask;
}
void Q_ISetSolo(void* d,uint32_t data)
{
    Q_State *Q = d;
    Q->SoloMask = data;
    Q_UpdateMuteMask(Q);
}
int Q_IGetVoiceCount(void* d)
{
    return Q_MAX_VOICES;
}
int Q_IGetVoiceInfo(void* d,int id,struct QP_DriverVoiceInfo *I)
{
    Q_State* Q = d;
    Q_Voice* V = &Q->Voice[id];
    memset(I,0,sizeof(*I));

    if(id>=Q_MAX_VOICES)
        return -1;

    // temporary until i add a key on flag
    I->Status = 0;
    I->Track = 0;
    if(V->TrackNo)
    {
        I->Status|=VOICE_STATUS_ACTIVE;
        I->Track = V->TrackNo-1;
    }
    if(V->Enabled==1)
        I->Status|=VOICE_STATUS_PLAYING;
    I->Channel = V->ChannelNo;
    I->VoiceType = VOICE_TYPE_MELODY|VOICE_TYPE_PCM;
    I->PanType = PAN_TYPE_SIGNED;
    I->VolumeMod = V->TrackVol ? V->Volume+*V->TrackVol : V->Volume;
    I->Volume = V->Volume;
    if(I->VolumeMod > 255)
        I->VolumeMod = 255;
    if(I->Volume > 255)
        I->Volume = 255;
    I->Key = V->BaseNote-2;
    I->Pitch = V->Pitch+V->PitchEnvMod+V->LfoMod-0x200;
    I->Preset = V->WaveNo&0x1fff;
    switch(V->PanMode)
    {
    case Q_PANMODE_IMM:
    default:
        I->Pan = (int8_t)V->Pan;
        break;
    case Q_PANMODE_ENV:
    case Q_PANMODE_ENV_SET:
        I->Pan = (int8_t)((V->PanEnvTarget-V->PanEnvValue)>>8);
        break;
    case Q_PANMODE_REG:
    case Q_PANMODE_POSREG:
        if(V->PanSource)
            I->Pan = (int8_t)(*V->PanSource & 0xff);
        break;
    }
    return 0;
}

struct QP_DriverInterface Q_CreateInterface()
{
    struct QP_DriverInterface d = {
        .Name = "Quattro",

        .Type = DRIVER_QUATTRO,

        // IInit is allowed to fail, this aborts the program
        .IInit = Q_IInit,

        .IDeinit = &Q_IDeinit,
        .IVgmOpen = &Q_IVgmOpen,
        .IVgmClose = &Q_IVgmClose,
        .IReset = &Q_IReset,

        .IGetParamCnt = &Q_IGetParamCnt,
        .ISetParam = &Q_ISetParam,
        .IGetParam = &Q_IGetParam,

        .IGetParamName = &Q_IGetParamName,
        .IGetSongMessage = &Q_IGetSongMessage,
        .IGetDriverInfo = &Q_IGetDriverInfo,

        .IRequestSlotCnt = &Q_IRequestSlotCnt,
        .ISongCnt = &Q_ISongCnt,
        .ISongRequest = &Q_ISongRequest,
        .ISongStop = &Q_ISongStop,
        .ISongFade = &Q_ISongFade,
        .ISongStatus = &Q_ISongStatus,
        .ISongId = &Q_ISongId,
        .ISongTime = &Q_ISongTime,

        .IGetLoopCnt = &Q_IGetLoopCnt,
        .IResetLoopCnt = &Q_IResetLoopCnt,

        .IDetectSilence = &Q_IDetectSilence,

        .ITickRate = &Q_ITickRate,
        .IUpdateTick = &Q_IUpdateTick,
        .IChipRate = &Q_IChipRate,
        .IUpdateChip = &Q_IUpdateChip,
        .ISampleChip = &Q_ISampleChip,

        .IGetMute = &Q_IGetMute,
        .ISetMute = &Q_ISetMute,
        .IGetSolo = &Q_IGetSolo,
        .ISetSolo = &Q_ISetSolo,

        .IGetVoiceCount = &Q_IGetVoiceCount,
        .IGetVoiceInfo = &Q_IGetVoiceInfo,
    };
    return d;
}
