#include "stdlib.h"
#include "string.h"
#include "qp.h"
#include "lib/vgm.h"

const struct QP_DriverTable DriverTable[DRIVER_COUNT] = {
    {0,"none"},
    {DRIVER_QUATTRO,"quattro"},
    {DRIVER_SYSTEM2,"system2x"},
};

int DriverCreate(struct QP_DriverInterface *di,enum QP_DriverType dt)
{
    switch(dt)
    {
    case DRIVER_QUATTRO:
        *di = Q_CreateInterface();

        di->Driver.quattro = (Q_State*)malloc(sizeof(Q_State));
        if(!di->Driver.quattro)
            return -1;
        memset(di->Driver.quattro,0,sizeof(Q_State));
        break;
    case DRIVER_SYSTEM2:
        *di = S2X_CreateInterface();

        di->Driver.s2x = (S2X_State*)malloc(sizeof(S2X_State));
        if(!di->Driver.s2x)
            return -1;

        memset(di->Driver.s2x,0,sizeof(S2X_State));
        break;
    default:
        return -1;
    }
    return 0;
}

void DriverDestroy(struct QP_DriverInterface *di)
{
    if(!di)
        return;
    if(di->Driver.drv)
        free(di->Driver.drv);
}

// Driver initialization
int DriverInit()
{
    return DriverInterface->IInit(DriverInterface->Driver,Game);
}
void DriverDeinit()
{
    return DriverInterface->IDeinit(DriverInterface->Driver);
}

// VGM open/close
void DriverInitVgm() // datablocks
{
    return DriverInterface->IVgmOpen(DriverInterface->Driver);
}
void DriverCloseVgm() // header/clocks
{
    return DriverInterface->IVgmClose(DriverInterface->Driver);
}

// Driver reset
void DriverReset(int initial)
{
    return DriverInterface->IReset(DriverInterface->Driver,Game,initial);
}

// Driver Parameters
int DriverGetParameterCount()
{
    return DriverInterface->IGetParamCnt(DriverInterface->Driver);
}
void DriverSetParameter(int id, int value)
{
    return DriverInterface->ISetParam(DriverInterface->Driver,id,value);
}
int DriverGetParameter(int id)
{
    return DriverInterface->IGetParam(DriverInterface->Driver,id);
}
int DriverGetParameterName(int id,char* buffer,int len)
{
    return DriverInterface->IGetParamName(DriverInterface->Driver,id,buffer,len);
}
char* DriverGetSongMessage()
{
    return DriverInterface->IGetSongMessage(DriverInterface->Driver);
}
char* DriverGetDriverInfo()
{
    return DriverInterface->IGetDriverInfo(DriverInterface->Driver);
}


// Song requests
int DriverGetSlotCount()
{
    return DriverInterface->IRequestSlotCnt(DriverInterface->Driver);
}
int DriverGetSongCount(int slot)
{
    return DriverInterface->ISongCnt(DriverInterface->Driver,slot);
}
void DriverRequestSong(int slot, int id)
{
    return DriverInterface->ISongRequest(DriverInterface->Driver,slot,id);
}
void DriverStopSong(int slot)
{
    return DriverInterface->ISongStop(DriverInterface->Driver,slot);
}
void DriverFadeOutSong(int slot)
{
    return DriverInterface->ISongFade(DriverInterface->Driver,slot);
}
int DriverGetSongStatus(int slot)
{
    return DriverInterface->ISongStatus(DriverInterface->Driver,slot);
}
int DriverGetSongId(int slot)
{
    return DriverInterface->ISongId(DriverInterface->Driver,slot);
}
double DriverGetPlayingTime(int slot)
{
    return DriverInterface->ISongTime(DriverInterface->Driver,slot);
}

// Loop detection
int DriverGetLoopCount(int slot)
{
    return DriverInterface->IGetLoopCnt(DriverInterface->Driver,slot);
}
void DriverResetLoopCount()
{
    return DriverInterface->IResetLoopCnt(DriverInterface->Driver);
}

// silence detection - return 1 if all voices are off
// doesn't actually detect silence, just inactivity
int DriverDetectSilence()
{
    return DriverInterface->IDetectSilence(DriverInterface->Driver);
}

double DriverGetTickRate()
{
    return DriverInterface->ITickRate(DriverInterface->Driver);
}
void DriverUpdateTick()
{
    return DriverInterface->IUpdateTick(DriverInterface->Driver);
}
double DriverGetChipRate()
{
    return DriverInterface->IChipRate(DriverInterface->Driver);
}
void DriverUpdateChip()
{
    return DriverInterface->IUpdateChip(DriverInterface->Driver);
}
// fetch samples
void DriverSampleChip(float* samples, int samplecnt)
{
    return DriverInterface->ISampleChip(DriverInterface->Driver,samples,samplecnt);
}

// get mute/solo masks
uint32_t DriverGetMute()
{
    return DriverInterface->IGetMute(DriverInterface->Driver);
}
void DriverSetMute(uint32_t data)
{
    return DriverInterface->ISetMute(DriverInterface->Driver,data);
}
uint32_t DriverGetSolo()
{
    return DriverInterface->IGetSolo(DriverInterface->Driver);
}
void DriverSetSolo(uint32_t data)
{
    return DriverInterface->ISetSolo(DriverInterface->Driver,data);
}
// reset mute and solo masks - convenience function
void DriverResetMute()
{
    DriverSetMute(0);
    DriverSetSolo(0);
}

void DriverDebugAction(int id)
{
    if(DriverInterface->IDebugAction)
        return DriverInterface->IDebugAction(DriverInterface->Driver,id);
}

int DriverGetVoiceCount()
{
    if(DriverInterface->IGetVoiceCount)
        return DriverInterface->IGetVoiceCount(DriverInterface->Driver);
    return 0;
}
int DriverGetVoiceInfo(int voice,struct QP_DriverVoiceInfo *dv)
{
    if(DriverInterface->IGetVoiceInfo)
        return DriverInterface->IGetVoiceInfo(DriverInterface->Driver,voice,dv);
    return -1;
}
