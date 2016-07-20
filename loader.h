#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED
#include "stdint.h"
#include "drv/quattro.h"

    char L_IniPath[128];
    char L_WavePath[128];
    char L_DataPath[128];

    char L_GameTitle[128];

    uint8_t *L_Data;
    uint32_t L_DataSize;
    uint8_t *L_WaveData;
    uint32_t L_WaveMask;

int LoadGame(Q_State* Q,char* gamename);
int UnloadGame(Q_State* Q);

#endif // LOADER_H_INCLUDED
