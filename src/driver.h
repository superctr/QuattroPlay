#ifndef DRIVER_H_INCLUDED
#define DRIVER_H_INCLUDED
#include <stdint.h>
#include "loader.h"

enum QP_DriverType {
    DRIVER_NOT_LOADED = 0,
    DRIVER_QUATTRO,
    DRIVER_SYSTEM2,
    DRIVER_COUNT
};
// bit mask, same values as quattro....
enum {
    SONG_STATUS_NOT_PLAYING = 0,
    SONG_STATUS_PLAYING = 0x8000,
    SONG_STATUS_STARTING = 0x4000,
    SONG_STATUS_FADEOUT = 0x2000,
    SONG_STATUS_SUBSONG = 0x0800,
};
enum {
    DEBUG_ACTION_DISPLAY_INFO = 0
};

union QP_Driver {
    void* drv;
    Q_State *quattro;
    S2X_State *s2x;
};

struct QP_DriverInterface {
    char* Name;

    enum QP_DriverType Type;
    union QP_Driver Driver;

    // IInit is allowed to fail, this aborts the program
    int (*IInit)(union QP_Driver,QP_Game *game);

    void (*IDeinit)(union QP_Driver);
    void (*IVgmOpen)(union QP_Driver);
    void (*IVgmClose)(union QP_Driver);
    void (*IReset)(union QP_Driver,QP_Game *game,int initial);

    int (*IGetParamCnt)(union QP_Driver);
    void (*ISetParam)(union QP_Driver,int id,int val);
    int (*IGetParam)(union QP_Driver,int id);

    int (*IGetParamName)(union QP_Driver,int id,char* buffer,int len);
    char* (*IGetSongMessage)(union QP_Driver);
    char* (*IGetDriverInfo)(union QP_Driver);

    int (*IRequestSlotCnt)(union QP_Driver);
    int (*ISongCnt)(union QP_Driver,int slot);
    void (*ISongRequest)(union QP_Driver,int slot,int val);
    void (*ISongStop)(union QP_Driver,int slot);
    void (*ISongFade)(union QP_Driver,int slot);
    int (*ISongStatus)(union QP_Driver,int slot);
    int (*ISongId)(union QP_Driver,int slot);
    double (*ISongTime)(union QP_Driver,int slot);

    int (*IGetLoopCnt)(union QP_Driver,int slot);
    void (*IResetLoopCnt)(union QP_Driver);

    int (*IDetectSilence)(union QP_Driver);

    double (*ITickRate)(union QP_Driver);
    void (*IUpdateTick)(union QP_Driver);
    double (*IChipRate)(union QP_Driver);
    void (*IUpdateChip)(union QP_Driver);
    void (*ISampleChip)(union QP_Driver,float* samples,int samplecnt);

    uint32_t (*IGetMute)(union QP_Driver);
    void (*ISetMute)(union QP_Driver,uint32_t data);
    uint32_t (*IGetSolo)(union QP_Driver);
    void (*ISetSolo)(union QP_Driver,uint32_t data);

    void (*IDebugAction)(union QP_Driver,int id);
};

struct QP_DriverTable {
    enum QP_DriverType type;
    char* name;
};

const struct QP_DriverTable DriverTable[DRIVER_COUNT];
int DriverCreate(struct QP_DriverInterface *di,enum QP_DriverType dt);
void DriverDestroy(struct QP_DriverInterface *di);

int DriverInit();
void DriverDeinit();
void DriverInitVgm();
void DriverCloseVgm();
void DriverReset(int initial);
int DriverGetParameterCount();
void DriverSetParameter(int id, int value);
int DriverGetParameter(int id);
int DriverGetParameterName(int id,char* buffer,int len);
char* DriverGetSongMessage();
char* DriverGetDriverInfo();
int DriverGetSlotCount();
int DriverGetSongCount(int slot);
void DriverRequestSong(int slot, int id);
void DriverStopSong(int slot);
void DriverFadeOutSong(int slot);
int DriverGetSongStatus(int slot);
int DriverGetSongId(int slot);
double DriverGetPlayingTime(int slot);
int DriverGetLoopCount(int slot);
void DriverResetLoopCount();
int DriverDetectSilence();
double DriverGetTickRate();
void DriverUpdateTick();
double DriverGetChipRate();
void DriverUpdateChip();
void DriverSampleChip(float* samples, int samplecnt);
uint32_t DriverGetMute();
void DriverSetMute(uint32_t data);
uint32_t DriverGetSolo();
void DriverSetSolo(uint32_t data);
void DriverResetMute();
void DriverDebugAction(int id);

#endif // DRIVER_H_INCLUDED
