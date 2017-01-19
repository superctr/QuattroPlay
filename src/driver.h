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
// bit mask, mostly same values as quattro....
enum {
    SONG_STATUS_NOT_PLAYING = 0,
    SONG_STATUS_STOPPING = 0x10000,
    SONG_STATUS_PLAYING = 0x8000,
    SONG_STATUS_STARTING = 0x4000,
    SONG_STATUS_FADEOUT = 0x2000,
    SONG_STATUS_SUBSONG = 0x0800,
};
enum {
    DEBUG_ACTION_DISPLAY_INFO = 0
};
enum {
    VOICE_STATUS_ACTIVE = 1,
    VOICE_STATUS_PLAYING = 2
};
enum {
    VOICE_TYPE_NONE = 0,
    VOICE_TYPE_MELODY = 1,
    VOICE_TYPE_PERCUSSION = 2,
    VOICE_TYPE_PCM = 0x10
};
enum {
    PAN_TYPE_NONE = 0,
    PAN_TYPE_SIGNED,
    PAN_TYPE_UNSIGNED,
    PAN_TYPE_INVERT = 0x10, // negative values = right
};

struct QP_DriverVoiceInfo {
    int Status;

    int Track;
    int Channel;

    int VoiceType; // Decides whether to display "Instrument", "Waveform" or PCM.
    int Preset;

    uint8_t  Key;
    uint16_t Pitch; // key + modifiers (vibrato, glissando, etc)

    uint16_t Volume; // volume (This is typically 0-255 but could be higher)
    uint16_t VolumeMod; // volume + modifiers (channel volume, etc)

    int PanType; // whether to display pan as signed or unsigned + invert flag
    int Pan;
};

struct QP_DriverInterface {
    char* Name;

    enum QP_DriverType Type;
    //union QP_Driver Driver;
    void* Driver;

    // IInit is allowed to fail, this aborts the program
    int (*IInit)(void*,QP_Game *game);

    void (*IDeinit)(void*);
    void (*IVgmOpen)(void*);
    void (*IVgmClose)(void*);
    void (*IReset)(void*,QP_Game *game,int initial);

    int (*IGetParamCnt)(void*);
    void (*ISetParam)(void*,int id,int val);
    int (*IGetParam)(void*,int id);

    int (*IGetParamName)(void*,int id,char* buffer,int len);
    char* (*IGetSongMessage)(void*);
    char* (*IGetDriverInfo)(void*);

    int (*IRequestSlotCnt)(void*);
    int (*ISongCnt)(void*,int slot);
    void (*ISongRequest)(void*,int slot,int val);
    void (*ISongStop)(void*,int slot);
    void (*ISongFade)(void*,int slot);
    int (*ISongStatus)(void*,int slot);
    int (*ISongId)(void*,int slot);
    double (*ISongTime)(void*,int slot);

    int (*IGetLoopCnt)(void*,int slot);
    void (*IResetLoopCnt)(void*);

    int (*IDetectSilence)(void*);

    double (*ITickRate)(void*);
    void (*IUpdateTick)(void*);
    double (*IChipRate)(void*);
    void (*IUpdateChip)(void*);
    void (*ISampleChip)(void*,float* samples,int samplecnt);

    uint32_t (*IGetMute)(void*);
    void (*ISetMute)(void*,uint32_t data);
    uint32_t (*IGetSolo)(void*);
    void (*ISetSolo)(void*,uint32_t data);

    void (*IDebugAction)(void*,int id);

    int (*IGetVoiceCount)(void*);
    int (*IGetVoiceInfo)(void*,int voice,struct QP_DriverVoiceInfo *dv);
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
int DriverGetVoiceCount();
int DriverGetVoiceInfo(int voice,struct QP_DriverVoiceInfo *dv);

#endif // DRIVER_H_INCLUDED
