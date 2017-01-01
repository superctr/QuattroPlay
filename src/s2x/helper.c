#include "../qp.h"
#include "helper.h"

uint8_t S2X_ReadByte(S2X_State *S,uint32_t d)
{
    return S->Data[d];
}

uint16_t S2X_ReadWord(S2X_State *S,uint32_t d)
{
    return ((S->Data[d]<<8) | (S->Data[d+1]<<0));
}

void S2X_UpdateMuteMask(S2X_State *S)
{
    if(S->SoloMask)
        S->PCMChip.mute_mask = ~(S->SoloMask);
    else
        S->PCMChip.mute_mask = S->MuteMask;
}
