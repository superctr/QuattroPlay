#ifndef S2X_HELPER_H_INCLUDED
#define S2X_HELPER_H_INCLUDED

uint32_t S2X_ReadPos(S2X_State *S,uint32_t d);
uint8_t S2X_ReadByte(S2X_State *S,uint32_t d);
uint16_t S2X_ReadWord(S2X_State *S,uint32_t d);
// uint16_t S2X_ReadWordBE(Q_State *Q,uint32_t d);
void S2X_UpdateMuteMask(S2X_State *S);

#endif // S2X_HELPER_H_INCLUDED
