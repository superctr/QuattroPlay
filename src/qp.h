/*
    QuattroPlay globals
*/
#ifndef QP_H_INCLUDED
#define QP_H_INCLUDED
#include "drv/quattro.h"
#include "audio.h"
#include "loader.h"

    char QP_IniPath[128];
    char QP_WavePath[128];
    char QP_DataPath[128];

    Q_State *QDrv;
    audio_t *Audio;
    game_t  *Game;

#endif // QP_H_INCLUDED
