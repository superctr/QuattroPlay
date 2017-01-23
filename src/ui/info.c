/*
    User interface code for QuattroPlay

    Sorry for the unreadability of this code, it's very much a quick and dirty job...
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "../qp.h"
#include "ui.h"
#include "scr_main.h"

void ui_info_track(int id,int ypos)
{
    switch(DriverInterface->Type)
    {
    case DRIVER_QUATTRO:
        return ui_info_q_track(id,ypos);
    default:
        break;
    }
}

void ui_info_voice(int id,int ypos)
{
    switch(DriverInterface->Type)
    {
    case DRIVER_QUATTRO:
        return ui_info_q_voice(id,ypos);
    default:
        break;
    }
}
