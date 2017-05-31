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

    // Deinitializes a sound driver. Any and all allocated memory should be
    // freed by this call
    void (*IDeinit)(void*);
    // Setup vgm logging for this sound driver
    void (*IVgmOpen)(void*);
    void (*IVgmClose)(void*);
    // Reset the sound driver. Initial is set to 1 for the initial setup (right after IInit)
    void (*IReset)(void*,QP_Game *game,int initial);

    // Driver parameters, bank switch, registers, conditional jumps etc.
    // Get parameter count.
    int (*IGetParamCnt)(void*);
    void (*ISetParam)(void*,int id,int val);
    int (*IGetParam)(void*,int id);
    // Return a user-friendly name for the parameter
    int (*IGetParamName)(void*,int id,char* buffer,int len);

    // Return a song message / song title.
    char* (*IGetSongMessage)(void*);
    // Return some user-friendly description of the sound driver
    char* (*IGetDriverInfo)(void*);

    // Get request slot count. Slots do not have to be symmetrical.
    int (*IRequestSlotCnt)(void*);
    // Get the amount of valid song IDs for that slot.
    int (*ISongCnt)(void*,int slot);
    // Request a song in the specified slot.
    void (*ISongRequest)(void*,int slot,int val);
    // Stop a song in the specified slot
    void (*ISongStop)(void*,int slot);
    // Fade out a song in the specified slot
    void (*ISongFade)(void*,int slot);
    // Get song status. See enum above for bit description. Bits 0-11 = song id.
    int (*ISongStatus)(void*,int slot);
    // as above, but just gets the song ID.
    int (*ISongId)(void*,int slot);
    // Get currently playing time for the slot, in seconds.
    double (*ISongTime)(void*,int slot);
    // Get the loop count for the specified slot.
    int (*IGetLoopCnt)(void*,int slot);
    // Reset the loop count for all slots
    void (*IResetLoopCnt)(void*);

    // Return zero if all voices are silent
    int (*IDetectSilence)(void*);

    // Get driver tick rate, in Hz
    double (*ITickRate)(void*);
    // Driver tick
    void (*IUpdateTick)(void*);
    // Get audio tick rate, in Hz
    double (*IChipRate)(void*);
    // Audio tick
    void (*IUpdateChip)(void*);
    // Get samples from the audio
    void (*ISampleChip)(void*,float* samples,int samplecnt);

    // Channel mute bitmask
    uint32_t (*IGetMute)(void*);
    void (*ISetMute)(void*,uint32_t data);
    // Channel isolation bitmask
    uint32_t (*IGetSolo)(void*);
    void (*ISetSolo)(void*,uint32_t data);

    void (*IDebugAction)(void*,int id);

    int (*IGetVoiceCount)(void*);
    int (*IGetVoiceInfo)(void*,int voice,struct QP_DriverVoiceInfo *dv);
    uint16_t (*IGetVoiceStatus)(void*,int voice); // returns less info than the above
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
uint16_t DriverGetVoiceStatus(int voice);
#endif // DRIVER_H_INCLUDED
