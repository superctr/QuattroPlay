/*
    Quattro - internal helper functions
*/
#ifndef HELPER_H_INCLUDED
#define HELPER_H_INCLUDED

uint32_t Q_ReadPos(Q_State *Q,uint32_t d);
uint8_t Q_ReadByte(Q_State *Q,uint32_t d);
uint16_t Q_ReadWord(Q_State *Q,uint32_t d);
uint16_t Q_ReadWordBE(Q_State *Q,uint32_t d);

Q_McuType Q_GetMcuType(Q_State* Q);
void Q_GetMcuVer(Q_State* Q);

void Q_GetOffsets(Q_State* Q);

int32_t Q_GetSongPos(Q_State *Q,uint16_t id);

uint16_t Q_GetRandom(uint16_t* lfsr);

#endif // HELPER_H_INCLUDED
