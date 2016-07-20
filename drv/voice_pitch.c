/*
    Quattro - pitch functions
*/
#include "quattro.h"
#include "voice.h"
#include "wave.h"
#include "helper.h"
#include "tables.h"

// call 0x20 - update portamento
// source: 0x7040
void Q_VoicePortaUpdate(Q_State *Q,int VoiceNo,Q_Voice *V)
{
    if(V->Pitch == V->PitchTarget)
        return;

    int16_t difference = V->PitchTarget - V->Pitch;
    int16_t step = difference>>8;

    if(difference < 0)
        step--;
    else
        step++;

    V->Pitch += (step*V->Portamento)/2;

    if(((V->PitchTarget-V->Pitch) ^ difference) < 0)
        V->Pitch = V->PitchTarget;
}

// reset pitch envelope (split from Q_VoiceKeyOn)
// source: 0x64c2
void Q_VoicePitchEnvSet(Q_State *Q,int VoiceNo,Q_Voice *V)
{
    if(!V->PitchEnvNo)
        return;

    V->PitchEnvPos = Q_ReadWord(Q, Q->TableOffset[Q_TOFFSET_PITCHTABLE]+(2*(V->PitchEnvNo-1)));

    if(Q->McuHeaderPos == 0x8000 && V->PitchEnvPos < 0x8000)
        V->PitchEnvPos += 0x10000;

    V->PitchEnvPos += Q->McuDataPosBase;

    V->PitchEnvSpeed = Q_ReadByte(Q,V->PitchEnvPos++);
    V->PitchEnvDepth = Q_ReadByte(Q,V->PitchEnvPos++);
    V->PitchEnvLoop = V->PitchEnvPos;
    V->PitchEnvData = Q_ReadByte(Q,V->PitchEnvPos++);
    V->PitchEnvCounter=0;
}

// call 0x1e - update pitch envelope
// source: 0x70bc
void Q_VoicePitchEnvUpdate(Q_State *Q,int VoiceNo,Q_Voice *V)
{
    if(!V->PitchEnvNo)
        return;

    uint16_t counter = V->PitchEnvCounter+V->PitchEnvSpeed;
    uint16_t step;
    int16_t target;
    V->PitchEnvCounter = counter&0xff;
    if(counter>0xff)
        V->PitchEnvData = Q_ReadByte(Q,V->PitchEnvPos++);
    counter&=0xff;

    target = Q_ReadByte(Q,V->PitchEnvPos);
    if(target==0xfd)
    {
        // continue reading next envelope
        V->PitchEnvPos++;
        V->PitchEnvSpeed = Q_ReadByte(Q,V->PitchEnvPos++);
        V->PitchEnvDepth = Q_ReadByte(Q,V->PitchEnvPos++);
        V->PitchEnvLoop = V->PitchEnvPos;
        return;
    }
    else if(target==0xfe)
    {
        // loop
        V->PitchEnvPos = V->PitchEnvLoop;
        V->PitchEnvCounter=0;
        return;
    }
    else if(target>=0xf0)
    {
        // end
        V->PitchEnvValue=V->PitchEnvData<<8;
        Q_VoicePitchEnvSetMod(Q,VoiceNo,V);
        V->PitchEnvNo=0;
        return;
    }

    step = (target-V->PitchEnvData)*counter;
    V->PitchEnvValue = (V->PitchEnvData<<8) + step;
    return Q_VoicePitchEnvSetMod(Q,VoiceNo,V);
}

// set pitch envelope frequency modulation
// source: 0x719e
void Q_VoicePitchEnvSetMod(Q_State *Q,int VoiceNo,Q_Voice *V)
{
    uint8_t depth = V->PitchEnvDepth;
    int16_t val = (V->PitchEnvValue-0x6400)>>1; // 100<<8
    int32_t mod = (val*depth)>>8;

    //printf("V=%02x pitch env %02x %04x %08x\n",VoiceNo,depth,(uint16_t)val,(uint32_t)mod);
    V->PitchEnvMod = mod&0xffff;
}
