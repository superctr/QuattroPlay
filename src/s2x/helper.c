#include "../qp.h"
#include "helper.h"
#include "../lib/vgm.h"

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
    {
        S->PCMChip.mute_mask = ~(S->SoloMask);
        S->FMChip.mute_mask = ~(S->SoloMask)>>24;
    }
    else
    {
        S->PCMChip.mute_mask = S->MuteMask;
        S->FMChip.mute_mask = S->MuteMask>>24;
    }
}

void S2X_OPMWrite(S2X_State *S,int ch,int op,int reg,uint8_t data)
{
    ch&=7;
    int fmreg = reg;
    if(reg >= 0x40)
        fmreg += (op*8)+ch;
    else if(reg >= 0x20)
        fmreg += ch;
    else if(reg == 0x08)
        data |= ch;

    if(S->PCMChip.vgm_log)
        vgm_write(0x54,0,fmreg,data);

    //printf(" -FM write %02x = %02x (Ch %d Op %d Reg %02x)\n",fmreg,data,ch,op,reg);
    return YM2151_write_reg(&S->FMChip,fmreg,data);
    // there really should be a delay for a few frames to let the sound chip breathe....
    // i plan to implement an FM command queue, this should solve the problem of popping
    // when starting a new note
}
