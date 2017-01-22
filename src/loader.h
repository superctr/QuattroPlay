#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#include <stdint.h>

#define GAME_CONFIG_MAX 256

typedef struct {
    int cnt;
    uint16_t reg[32];
    uint16_t data[32];
} QP_GameAction;

typedef struct {
    int wait_type;
    int wait_count;
    int action_id;
} QP_PlaylistScript;

typedef struct {
    int SongID;
    int Bank;
    char Title[256];
    QP_PlaylistScript script[16];
} QP_PlaylistEntry;

typedef struct {
    char name[16];
    char data[48];
} QP_GameConfig;

typedef struct {

    char Name[256]; // short name (filename-legal)
    char Title[1024]; // display title
    char Type[64]; // driver type

    uint8_t *Data;
    uint32_t DataSize;
    uint8_t *WaveData;
    uint32_t WaveMask;

    //Q_State *QDrv;

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

    QP_GameAction Action[256];
    QP_GameConfig Config[GAME_CONFIG_MAX];
    int ConfigCount;

    int SongCount;
    QP_PlaylistEntry Playlist[256];

    int PlaylistControl; // 0=user control, 1=playlist control
    int PlaylistPosition;
    int PlaylistScript;
    int PlaylistSongID;
    int PlaylistLoop;

    int QueueSong;
    int QueueAction;
    int ActionTimer;
} QP_Game;

int LoadGame(QP_Game *Game);
int UnloadGame(QP_Game *Game);

int  InitGame(QP_Game *Game);
void DeInitGame(QP_Game *Game);

void GameDoAction(QP_Game *G,unsigned int actionid);
void GameDoUpdate(QP_Game *G);

#endif // LOADER_H_INCLUDED
