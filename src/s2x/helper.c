#include <stdlib.h>
#include <string.h>

#include "../qp.h"
#include "../lib/vgm.h"

#include "s2x.h"
#include "helper.h"

#define SYSTEMNA (S->DriverType == S2X_TYPE_NA)

uint8_t S2X_ReadByte(S2X_State *S,uint32_t d)
{
    return S->Data[d];
}

uint16_t S2X_ReadWord(S2X_State *S,uint32_t d)
{
    if(SYSTEMNA) // little endian
        return ((S->Data[d+1]<<8) | (S->Data[d]<<0));
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

    S2X_FMWrite w = {fmreg,data};

    //Q_DEBUG("write queue %02x (%02x %02x)\n",S->FMQueueWrite,fmreg,data);
    S->FMQueue[(S->FMQueueWrite++)&0x1ff] = w;

    // flush the queue!
    if((S->FMQueueWrite&0x1ff) == (S->FMQueueRead&0x1ff))
    {
        Q_DEBUG("flushing queue (OPM is not keeping up!)\n");
        do S2X_OPMReadQueue(S);
        while ((S->FMQueueWrite&0x1ff) != (S->FMQueueRead&0x1ff));
    }

    //printf(" -FM write %02x = %02x (Ch %d Op %d Reg %02x)\n",fmreg,data,ch,op,reg);
    //return YM2151_write_reg(&S->FMChip,fmreg,data);
    // there really should be a delay for a few frames to let the sound chip breathe....
    // i plan to implement an FM command queue, this should solve the problem of popping
    // when starting a new note
}

void S2X_OPMReadQueue(S2X_State *S)
{
    //Q_DEBUG("read  queue %02x (%02x %02x)\n",S->FMQueueRead,S->FMQueue[S->FMQueueRead].Reg,S->FMQueue[S->FMQueueRead].Data);
    S2X_FMWrite* w = &S->FMQueue[(S->FMQueueRead++)&0x1ff];
    return YM2151_write_reg(&S->FMChip,w->Reg,w->Data);
}

int S2X_LoopDetectValid(void* drv,int trackno)
{
    S2X_State *S = drv;
    if(S->Track[trackno].RepeatStackPos || S->Track[trackno].LoopStackPos)
        return -1;
    return 0;
}

void S2X_ReadConfig(S2X_State *S,QP_Game *G)
{
    S->DriverType=S2X_TYPE_SYSTEM2;
    S->ConfigFlags=0;
    S->FMBase = 0;
    S->PCMBase = 0;

    Q_DEBUG("driver type = %s\n",G->Type);

    if(!strcmp(G->Type,"System1"))
    {
        S->DriverType=S2X_TYPE_SYSTEM1;
        S->ConfigFlags|=S2X_CFG_SYSTEM1|S2X_CFG_FM_VOL;
    }
    else if(!strcmp(G->Type,"NA"))
    {
        S->DriverType=S2X_TYPE_NA;
    }

    QP_GameConfig *cfg;

    int bankid=0;
    int src=0,dst=0,len=0;

    int i,v;
    for(i=0;i<G->ConfigCount;i++)
    {
        cfg = &G->Config[i];
        v = atoi(cfg->data);
        if(!strcmp(cfg->name,"fm_volcalc") && v)
            S->ConfigFlags |= S2X_CFG_FM_VOL;
        else if(!strcmp(cfg->name,"pcm_adsr") && v>1)
            S->ConfigFlags |= S2X_CFG_PCM_NEWADSR;
        else if(!strcmp(cfg->name,"pcm_adsr") && v>0)
            S->ConfigFlags |= S2X_CFG_PCM_ADSR;
        else if(!strcmp(cfg->name,"pcm_paninvert") && v)
            S->ConfigFlags |= S2X_CFG_PCM_PAN;
        else if(!strcmp(cfg->name,"fm_paninvert") && v)
            S->ConfigFlags |= S2X_CFG_FM_PAN;
        else if(!strcmp(cfg->name,"fm_writerate"))
            S->FMWriteRate = atof(cfg->data);
        else if(!strcmp(cfg->name,"fm_base"))
            S->FMBase = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"pcm_base"))
            S->PCMBase = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"bank")) // can be omitted if only a single bank is used
            bankid = strtol(cfg->data,NULL,0) % S2X_MAX_BANK;
        else if(!strcmp(cfg->name,"src")) // source address
            src = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"dst")) // destination bank number
            dst = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"len")) // to make parsing easier we write the bank parameters here
        {
            len = strtol(cfg->data,NULL,0);
            Q_DEBUG("bank %02x src=%06x dst=%02x len=%02x\n",bankid,src,dst,len);
            for(v=dst;v<len;v++)
            {
               S->WaveBase[bankid][v] = src;
               src+=0x10000;
            }
        }
    }
}
