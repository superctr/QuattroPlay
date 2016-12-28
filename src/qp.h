/*
    QuattroPlay globals
*/
#ifndef QP_H_INCLUDED
#define QP_H_INCLUDED

#define QP_TITLE "QuattroPlay"
#define QP_VERSION "1.1"
#define QP_COPYRIGHT "2016 Ian Karlsson"
#define QP_LICENSE "GPL v2"
#define QP_WEBSITE "https://github.com/superctr/QuattroPlay"

#include "drv/quattro.h"
#include "driver.h"
#include "audio.h"
#include "loader.h"
#include "lib/audit.h"

    char QP_IniPath[128];
    char QP_WavePath[128];
    char QP_DataPath[128];

    Q_State *QDrv;
    audio_t *Audio;
    game_t  *Game;
    QPAudit *Audit;

    struct _DriverInterface *DriverInterface;

#endif // QP_H_INCLUDED
