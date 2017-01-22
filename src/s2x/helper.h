#ifndef S2X_HELPER_H_INCLUDED
#define S2X_HELPER_H_INCLUDED
#include "../macro.h"
#include "../qp.h"

uint32_t S2X_ReadPos(S2X_State *S,uint32_t d);
uint8_t S2X_ReadByte(S2X_State *S,uint32_t d);
uint16_t S2X_ReadWord(S2X_State *S,uint32_t d);
// uint16_t S2X_ReadWordBE(Q_State *Q,uint32_t d);
void S2X_UpdateMuteMask(S2X_State *S);
void S2X_OPMWrite(S2X_State *S,int ch,int op,int reg,uint8_t data);
void S2X_OPMReadQueue(S2X_State *S);

// loop detection callback
int S2X_LoopDetectValid(void* drv,int trackno);

#ifndef Q_DISABLE_LOOP_DETECTION
void S2X_LoopDetectionInit(S2X_State *S);
void S2X_LoopDetectionFree(S2X_State *S);
void S2X_LoopDetectionReset(S2X_State *S);
void S2X_LoopDetectionCheck(S2X_State *S,int TrackNo,int stopped);
void S2X_LoopDetectionJumpCheck(S2X_State *S,int TrackNo);
int8_t S2X_LoopDetectionGetCount(S2X_State *S,int TrackNo);
#else
#define S2X_LoopDetectionInit() 0
#define S2X_LoopDetectionFree() 0
#define S2X_LoopDetectionReset() 0
#define S2X_LoopDetectionCheck() 0
#define S2X_LoopDetectionJumpCheck() 0
#define S2X_LoopDetectionGetCount() 0
#endif // Q_DISABLE_LOOP_DETECTION

void S2X_ReadConfig(S2X_State *S,QP_Game *G);

#endif // S2X_HELPER_H_INCLUDED
