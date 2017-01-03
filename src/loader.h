#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED
#include "stdint.h"
#include "drv/quattro.h"

typedef struct {
    int cnt;
    uint16_t reg[32];
    uint16_t data[32];
} action_t;

typedef struct {
    int wait_type;
    int wait_count;
    int action_id;
} playlist_script_t;

typedef struct {
    int SongID;
    char Title[256];
    playlist_script_t script[16];
} playlist_entry_t;

typedef struct {

    char Name[256]; // short name (filename-legal)
    char Title[1024]; // display title
    char Type[64]; // driver type

    uint8_t *Data;
    uint32_t DataSize;
    uint8_t *WaveData;
    uint32_t WaveMask;

    Q_State *QDrv;

    // audio configuration
    char AudioDevice[256];
    int AudioBuffer;

    // Global configuration
    int WavLog;
    int VgmLog;
    int AutoPlay;
    int PortaFix;
    int BootSong;
    float BaseGain;

    // Game configuration
    float UIGain;
    float Fadeout;
    float Gain;
    int MuteRear;
    int ChipFreq; // sound chip frequency, best to not touch this.

    action_t Action[256];

    int SongCount;
    playlist_entry_t Playlist[256];

    int PlaylistControl; // 0=user control, 1=playlist control
    int PlaylistPosition;
    int PlaylistScript;
    int PlaylistSongID;
    int PlaylistLoop;

    int QueueSong;
    int QueueAction;
    int ActionTimer;
} game_t;

int LoadGame(game_t *Game);
int UnloadGame(game_t *Game);

int  InitGame(game_t *Game);
void DeInitGame(game_t *Game);

void GameDoAction(game_t *G,unsigned int actionid);
void GameDoUpdate(game_t *G);

#endif // LOADER_H_INCLUDED
