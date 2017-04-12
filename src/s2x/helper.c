#include <stdlib.h>
#include <string.h>

#include "../qp.h"
#include "../lib/vgm.h"

#include "s2x.h"
#include "helper.h"
#include "tables.h"
#include "voice.h"
#include "../drv/tables.h" /* quattro pitch table (used for NA-1/NA-2) */

#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)
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
    uint64_t dat;

    int i,v;
    for(i=0;i<G->ConfigCount;i++)
    {
        cfg = &G->Config[i];
        v = atoi(cfg->data);
        if(SYSTEM1 && !strcmp(cfg->name,"type") && v)
            S->DriverType=S2X_TYPE_SYSTEM1+(v%2);
        else if(!strcmp(cfg->name,"wsg_type") && v)
            S->ConfigFlags |= S2X_CFG_WSG_TYPE;
        else if(!strcmp(cfg->name,"wsg_cmd0b") && v)
            S->ConfigFlags |= S2X_CFG_WSG_CMD0B;
        else if(!strcmp(cfg->name,"fm_volcalc") && v)
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
        else if(!strcmp(cfg->name,"name")) // bank name
            S->BankName[bankid] = cfg->data;
        else if(!strcmp(cfg->name,"src")) // source address
            src = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"dst")) // destination bank number
            dst = strtol(cfg->data,NULL,0);
        else if(!strcmp(cfg->name,"len")) // to make parsing easier we write the bank parameters here
        {
            len = strtol(cfg->data,NULL,0);
            Q_DEBUG("bank %02x src=%06x dst=%02x len=%02x\n",bankid,src,dst,len);
            for(v=dst;v<dst+len;v++)
            {
               S->WaveBase[bankid][v] = src;
               src+=0x10000;
            }
        }
        else if(!strcmp(cfg->name,"blk")) // combined
        {
            dat = strtoll(cfg->data,NULL,0);
            dst = (dat>>32)&0xff;
            len = (dat>>24)&0xff;
            src = (dat&0xffffff);
            Q_DEBUG("bank %02x src=%06x dst=%02x len=%02x\n",bankid,src,dst,len);
            for(v=dst;v<dst+len;v++)
            {
               S->WaveBase[bankid][v] = src;
               src+=0x10000;
            }
        }
    }
}

uint8_t S2X_PitchTableBE[0x18] = {
    0x00, 0x54, 0x00, 0x59, 0x00, 0x5e, 0x00, 0x64,
    0x00, 0x6a, 0x00, 0x70, 0x00, 0x77, 0x00, 0x7e,
    0x00, 0x86, 0x00, 0x8e, 0x00, 0x96, 0x00, 0x9f
};

/*
 interesting, the envelope rate table seems to have been designed to
 extend the pitch table in the event of a pitch table underflow (unsigned
 so it's actually overflows). Solvalou takes advantage of this...
*/
void S2X_MakePitchTable(S2X_State *S)
{
    uint8_t *search = S2X_PitchTableBE;
    int i,j;
    if(SYSTEM1) // no need to get pitch table
        return;
    else if(SYSTEMNA)
    {
        // use quattro pitch table
        memcpy(S->PCMPitchTable,Q_PitchTable,108*sizeof(uint16_t));
        for(i=108;i<129;i++)
            S->PCMPitchTable[i]=0xffff;
    }
    else
    {
        for(i=0;i<0x4000;i++)
        {
            if(!memcmp(S->Data+i,search,0x18))
            {
                Q_DEBUG("Pitch table located at 0x%04x\n",i);
                for(j=0;j<129;j++)
                    S->PCMPitchTable[j] = S2X_ReadWord(S,i+(j<<1));
                return;
            }
        }
        Q_DEBUG("Pitch table not found... Using default pitch table\n");
        // use default pitch table.
        memcpy(S->PCMPitchTable,S2X_PitchTable,sizeof(S2X_PitchTable));
        // copy overflow values from envelope rate table
        for(i=96;i<129;i++)
            S->PCMPitchTable[i] = ((S2X_EnvelopeRateTable[i-96]&0xff)<<8)|((S2X_EnvelopeRateTable[i-96]>>8)&0xff);
    }
}

void S2X_InitDriverType(S2X_State *S)
{
    S2X_SetVoiceType(S,0,S2X_VOICE_TYPE_PCM,16);
    S2X_SetVoiceType(S,16,S2X_VOICE_TYPE_SE,8);
    S2X_SetVoiceType(S,24,S2X_VOICE_TYPE_FM,8);

    switch(S->DriverType)
    {
    case S2X_TYPE_SYSTEM1:
    case S2X_TYPE_SYSTEM1_ALT:
        if(!S->FMBase)
            S->FMBase = 0x10000;
        S->PCMBase=0x4000; // WSG
        S2X_SetVoiceType(S,0,S2X_VOICE_TYPE_WSG,8);
        break;
    case S2X_TYPE_NA:
        S->SongCount[0] = S2X_ReadByte(S,S->PCMBase+0x11);
        S->SongCount[1] = S2X_ReadByte(S,S->PCMBase+0x10011);
        Q_DEBUG("base = %06x\nmax(1) = %02x\nmax(2) = %02x\n",S->PCMBase,S->SongCount[0],S->SongCount[1]);
        break;
    case S2X_TYPE_SYSTEM2:
        if(!S->FMBase)
        {
            S->FMBase = 0x4000;
            // some early games have PCM and FM swapped
            if(S2X_ReadWord(S,0x10000) == 0x0008)
                S->FMBase=0x10000;
        }
        if(!S->PCMBase)
        {
            S->PCMBase = 0x10000;
            // some early games have PCM and FM swapped
            if(S2X_ReadWord(S,0x10000) == 0x0008)
                S->PCMBase=0x4000;
        }
        break;
    }
}

