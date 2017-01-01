#include "string.h"
#include "../qp.h"
#include "../lib/vgm.h"
#include "helper.h"

int Q_IInit(union _Driver d,game_t *g)
{
    memset(&d.quattro->Chip,0,sizeof(C352));

    d.quattro->McuType = Q_GetMcuTypeFromString(g->Type);
    d.quattro->ChipClock = g->ChipFreq;
    C352_init(&d.quattro->Chip,g->ChipFreq);
    d.quattro->Chip.mulaw_type = C352_MULAW_TYPE_C352;
    d.quattro->Chip.vgm_log = 0;

    d.quattro->Chip.wave = g->WaveData;
    d.quattro->Chip.wave_mask = g->WaveMask;
    d.quattro->McuData = g->Data;
    return 0;
}
void Q_IDeinit(union _Driver d)
{
    Q_Deinit(d.quattro);
}
void Q_IVgmOpen(union _Driver d)
{
    vgm_datablock(0x92,0x1000000,d.quattro->Chip.wave,0x1000000,d.quattro->Chip.wave_mask,0);
    d.quattro->Chip.vgm_log = 1;
}
void Q_IVgmClose(union _Driver d)
{
    d.quattro->Chip.vgm_log = 0;
    vgm_poke32(0xdc,d.quattro->ChipClock | Audio->state.MuteRear<<31);
    vgm_poke8(0xd6,288/4);
}
void Q_IReset(union _Driver d,game_t* g,int initial)
{
    d.quattro->PortaFix=g->PortaFix;
    d.quattro->BootSong=g->BootSong;

    if(initial)
        Q_Init(d.quattro);
    else
        Q_Reset(d.quattro);

    if(initial && g->AutoPlay >= 0)
        d.quattro->BootSong=2;
}

int Q_IGetParamCnt(union _Driver d)
{
    return 256;
}
void Q_ISetParam(union _Driver d,int id,int val)
{
    d.quattro->Register[id&0xff] = val;
}
int Q_IGetParam(union _Driver d,int id)
{
    return d.quattro->Register[id&0xff];
}
int Q_IGetParamName(union _Driver d,int id,char* buffer,int len)
{
    snprintf(buffer,len,"Register %02x",id);
    return -1;
}
char* Q_IGetSongMessage(union _Driver d)
{
    return d.quattro->SongMessage;
}
char* Q_IGetDriverInfo(union _Driver d)
{
    return (char*)Q_McuNames[d.quattro->McuType];
}
int Q_IRequestSlotCnt(union _Driver d)
{
    return Q_MAX_TRACKS;
}
int Q_ISongCnt(union _Driver d,int slot)
{
    return d.quattro->SongCount;
}
void Q_ISongRequest(union _Driver d,int slot,int val)
{
    d.quattro->SongRequest[slot&0x3f] = val | Q_TRACK_STATUS_START;
}
void Q_ISongStop(union _Driver d,int slot)
{
    d.quattro->SongRequest[slot&0x3f] &= 0x7ff;
}
void Q_ISongFade(union _Driver d,int slot)
{
    d.quattro->SongRequest[slot&0x3f] |= Q_TRACK_STATUS_FADE;
}
int Q_ISongStatus(union _Driver d,int slot)
{
    return (d.quattro->SongRequest[slot&0x3f] & 0xf800);
}
int Q_ISongId(union _Driver d,int slot)
{
    return d.quattro->SongRequest[slot&0x3f] & 0x7ff;
}
double Q_ISongTime(union _Driver d,int slot)
{
    return d.quattro->SongTimer[slot&0x3f];
}

int Q_IGetLoopCnt(union _Driver d,int slot)
{
    return Q_LoopDetectionGetCount(d.quattro,slot);
}
void Q_IResetLoopCnt(union _Driver d)
{
    Q_LoopDetectionReset(d.quattro);
}

int Q_IDetectSilence(union _Driver d)
{
    int i, voicectr=0;
    for(i=0;i<Q_MAX_VOICES;i++)
        if(!d.quattro->Voice[i].Enabled)
            voicectr++;
    if(voicectr != Q_MAX_VOICES)
        return 1;
    return 0;
}

double Q_ITickRate(union _Driver d)
{
    return 120; // 120 Hz
}
void Q_IUpdateTick(union _Driver d)
{
    Q_UpdateTick(d.quattro);
}
double Q_IChipRate(union _Driver d)
{
    return d.quattro->Chip.rate;
}
void Q_IUpdateChip(union _Driver d)
{
    C352_update(&d.quattro->Chip);
}
void Q_ISampleChip(union _Driver d,float* samples,int samplecnt)
{
    int i;
    if(samplecnt > 4)
        samplecnt=4;
    for(i=0;i<samplecnt;i++)
        samples[i] = d.quattro->Chip.out[i] / 268435456;
}

uint32_t Q_IGetMute(union _Driver d)
{
    return QDrv->MuteMask;
}
void Q_ISetMute(union _Driver d,uint32_t data)
{
    QDrv->MuteMask = data;
    Q_UpdateMuteMask(QDrv);
}
uint32_t Q_IGetSolo(union _Driver d)
{
    return QDrv->SoloMask;
}
void Q_ISetSolo(union _Driver d,uint32_t data)
{
    QDrv->SoloMask = data;
    Q_UpdateMuteMask(QDrv);
}

struct _DriverInterface Q_CreateInterface()
{
    struct _DriverInterface d = {
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
    };
    return d;
}
