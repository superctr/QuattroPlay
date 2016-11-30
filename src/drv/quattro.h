/*
    Quattro - public functions
*/
#ifndef QUATTRO_H_INCLUDED
#define QUATTRO_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#define Q_MAX_TRACKS 32
#define Q_MAX_REGISTER 256

#include "../c352.h"

#include "enum.h"
#include "struct.h"
#include "version.h"

#ifdef DEBUG
#define Q_DEBUG(...) printf(__VA_ARGS__)
#else
#define Q_DEBUG(...)
#endif

#define Q_C352_R(_q,_v,_r) C352_read(&_q->Chip,(_v<<3)|_r)
#define Q_C352_W(_q,_v,_r,_d) C352_write(&_q->Chip,(_v<<3)|_r,_d)

const char* Q_McuNames[Q_MCUTYPE_MAX];
const char* Q_NoteNames[12];

// Initialize driver
void Q_Init(Q_State* Q);
void Q_Deinit(Q_State* Q);

// Reset driver
void Q_Reset(Q_State* Q);

// Call every 1/120 second
void Q_UpdateTick(Q_State* Q);

// get MCU type from string...
Q_McuType Q_GetMcuTypeFromString(char* s);

// TODO: ReadTrackData/WriteTrackData sounds better
// Read byte from track struct
uint8_t Q_ReadTrackInfo(Q_State* Q, int trk, int index);
// Read byte from track channel struct
uint8_t Q_ReadChannelInfo(Q_State *Q, int trk, int ch, int index);

// Read byte from track struct
void Q_WriteTrackInfo(Q_State* Q, int trk, int index, uint8_t data);
// Read byte from track channel struct
void Q_WriteChannelInfo(Q_State *Q, int trk, int ch, int index, uint8_t data);
void Q_WriteMacroInfo(Q_State *Q, int macro, int index, uint8_t data);
void Q_UpdateMuteMask(Q_State *Q);

#ifndef Q_DISABLE_LOOP_DETECTION
void Q_LoopDetectionInit(Q_State *Q);
void Q_LoopDetectionFree(Q_State *Q);
void Q_LoopDetectionReset(Q_State *Q);
void Q_LoopDetectionCheck(Q_State *Q,int TrackNo,int stopped);
void Q_LoopDetectionJumpCheck(Q_State *Q,int TrackNo);
int8_t Q_LoopDetectionGetCount(Q_State *Q,int TrackNo);
#else
#define Q_LoopDetectionInit() 0
#define Q_LoopDetectionFree() 0
#define Q_LoopDetectionReset() 0
#define Q_LoopDetectionCheck() 0
#define Q_LoopDetectionJumpCheck() 0
#define Q_LoopDetectionGetCount() 0
#endif // Q_DISABLE_LOOP_DETECTION

#endif // QUATTRO_H_INCLUDED
