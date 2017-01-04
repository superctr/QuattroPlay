#ifndef S2X_H_INCLUDED
#define S2X_H_INCLUDED
/*
    System 2/21 sound driver
*/
#include <stdint.h>
#include "../emu/c352.h"
#include "../emu/ym2151.h"

#include "enum.h"
#include "struct.h"

// no C140 emulation yet
#define S2X_C352_R(_q,_v,_r) C352_read(&_q->PCMChip,(_v<<3)|_r)
#define S2X_C352_W(_q,_v,_r,_d) C352_write(&_q->PCMChip,(_v<<3)|_r,_d)

void S2X_Init(S2X_State *S);
void S2X_Deinit(S2X_State *S);
void S2X_Reset(S2X_State *S);
void S2X_UpdateTick(S2X_State *S);

struct QP_DriverInterface S2X_CreateInterface();

#endif // S2X_H_INCLUDED
