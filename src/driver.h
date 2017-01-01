#ifndef DRIVER_H_INCLUDED
#define DRIVER_H_INCLUDED
#include <stdint.h>
#include "loader.h"

enum _DriverType {
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

union _Driver {
    void* drv;
    Q_State *quattro;
    S2X_State *s2x;
};

struct _DriverInterface {
    char* Name;

    enum _DriverType Type;
    union _Driver Driver;

    // IInit is allowed to fail, this aborts the program
    int (*IInit)(union _Driver, game_t *game);

    void (*IDeinit)(union _Driver);
    void (*IVgmOpen)(union _Driver);
    void (*IVgmClose)(union _Driver);
    void (*IReset)(union _Driver,game_t *game,int initial);

    int (*IGetParamCnt)(union _Driver);
    void (*ISetParam)(union _Driver,int id,int val);
    int (*IGetParam)(union _Driver,int id);

    int (*IGetParamName)(union _Driver,int id,char* buffer,int len);
    char* (*IGetSongMessage)(union _Driver);
    char* (*IGetDriverInfo)(union _Driver);

    int (*IRequestSlotCnt)(union _Driver);
    int (*ISongCnt)(union _Driver,int slot);
    void (*ISongRequest)(union _Driver,int slot,int val);
    void (*ISongStop)(union _Driver,int slot);
    void (*ISongFade)(union _Driver,int slot);
    int (*ISongStatus)(union _Driver,int slot);
    int (*ISongId)(union _Driver,int slot);
    double (*ISongTime)(union _Driver,int slot);

    int (*IGetLoopCnt)(union _Driver,int slot);
    void (*IResetLoopCnt)(union _Driver);

    int (*IDetectSilence)(union _Driver);

    double (*ITickRate)(union _Driver);
    void (*IUpdateTick)(union _Driver);
    double (*IChipRate)(union _Driver);
    void (*IUpdateChip)(union _Driver);
    void (*ISampleChip)(union _Driver,float* samples,int samplecnt);

    uint32_t (*IGetMute)(union _Driver);
    void (*ISetMute)(union _Driver,uint32_t data);
    uint32_t (*IGetSolo)(union _Driver);
    void (*ISetSolo)(union _Driver,uint32_t data);
};

struct _DriverTable {
    enum _DriverType type;
    char* name;
};

const struct _DriverTable DriverTable[DRIVER_COUNT];
int DriverCreate(struct _DriverInterface *di,enum _DriverType dt);
void DriverDestroy(struct _DriverInterface *di);

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

#endif // DRIVER_H_INCLUDED
