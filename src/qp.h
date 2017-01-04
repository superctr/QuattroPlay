/*
    QuattroPlay globals
*/
#ifndef QP_H_INCLUDED
#define QP_H_INCLUDED

#define QP_TITLE "QuattroPlay"
#define QP_VERSION "1.1"
#define QP_COPYRIGHT "2016-2017 Ian Karlsson"
#define QP_LICENSE "GPL v2"
#define QP_WEBSITE "https://github.com/superctr/QuattroPlay"

#include "drv/quattro.h"
#include "s2x/s2x.h"
#include "driver.h"
#include "audio.h"
#include "loader.h"
#include "lib/audit.h"

    char QP_IniPath[128];
    char QP_WavePath[128];
    char QP_DataPath[128];

    Q_State *QDrv;
    QP_Audio *Audio;
    QP_Game  *Game;
    QP_Audit *Audit;

    struct QP_DriverInterface *DriverInterface;

#endif // QP_H_INCLUDED
