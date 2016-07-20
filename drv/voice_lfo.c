/*
    Quattro - LFO functions
*/
#include "quattro.h"
#include "voice.h"
#include "wave.h"
#include "helper.h"
#include "tables.h"

// reset LFO (split from Q_VoiceKeyOn)
// source: 0x676c
void Q_VoiceLfoSet(Q_State *Q,int VoiceNo,Q_Voice *V)
{
    if(!V->LfoNo)
    {
        V->LfoEnable=0;
        return;
    }

    uint32_t pos = Q->TableOffset[Q_TOFFSET_LFOTABLE]+(((V->LfoNo-1)&0xff)*8);

    V->LfoWaveform = Q_ReadByte(Q,pos++);
    V->LfoDelay = Q_ReadByte(Q,pos++);
    V->LfoFreq = Q_ReadByte(Q,pos++)<<8;
    V->LfoDepth = Q_ReadByte(Q,pos++)<<8;
    V->LfoFreqTarget = Q_ReadByte(Q,pos++)<<8;
    V->LfoDepthTarget = Q_ReadByte(Q,pos++)<<8;
    V->LfoFreqDelta = Q_EnvelopeRateTable[Q_ReadByte(Q,pos++)&0xff];
    V->LfoDepthDelta = Q_EnvelopeRateTable[Q_ReadByte(Q,pos++)&0xff];
    V->LfoPhase=0;
    V->LfoEnable=1;
}

// call 0x22 - LFO update function
// source: 0x6cf6, 0x6d04 (vector)
void Q_VoiceLfoUpdate(Q_State *Q,int VoiceNo,Q_Voice *V)
{

    if(!V->LfoEnable)
        return;
    if(V->LfoDelay)
    {
        V->LfoDelay--;
        return;
    }

    uint8_t wavepos;
    uint8_t delta;
    uint8_t depth;
    int16_t phase;

    V->LfoFreq = Q_VoiceLfoUpdateEnv(V->LfoFreq,V->LfoFreqTarget,V->LfoFreqDelta);
    V->LfoDepth = Q_VoiceLfoUpdateEnv(V->LfoDepth,V->LfoDepthTarget,V->LfoDepthDelta);
    uint8_t freq = V->LfoFreq>>8;

    switch(V->LfoWaveform)
    {
    case 0x0c: // sawtooth
        V->LfoPhase += freq;
        depth = V->LfoPhase;
        break;
    case 0x0d: // supposed to be random amplitude triangle
        // this isn't matching the original yet...
        depth = V->LfoPhase>>8;
        phase = V->LfoPhase&0xff;

        phase -= freq;
        wavepos = phase&0xff;
        if(phase < 0)
            depth = Q_VoiceLfoGetRandom(Q)>>1;
        if(wavepos & 0x80)
            wavepos = ~wavepos;
        V->LfoPhase = (depth<<8)|(phase&0xff);
        depth = wavepos;
        break;
    case 0x0e: // random 1
        V->LfoPhase++;
        if(V->LfoPhase != freq)
            return;
        depth = Q_VoiceLfoGetRandom(Q);
        break;
    case 0x0f: // random 2
        V->LfoPhase++;
        if(V->LfoPhase < 0x100)
            return;
        V->LfoPhase = Q_VoiceLfoGetRandom(Q);
        if(freq)
            V->LfoPhase /= freq;
        depth = Q_VoiceLfoGetRandom(Q);
        break;
    default: // get waveform from LUT
        wavepos = ((V->LfoWaveform&0x0f)<<4)|((V->LfoPhase&0x1e0)>>5);
        depth = (Q_LfoWaveTable[wavepos]&0xff00)>>8; // base
        delta = ((Q_LfoWaveTable[wavepos]&0x007f) * (V->LfoPhase&0x1f))>>4;
        //delta = V->LfoPhase>>3;
        if(Q_LfoWaveTable[wavepos]&0x80)
            depth -= delta;
        else
            depth += delta;
        V->LfoPhase += freq;

        break;
    }
    return Q_VoiceLfoSetMod(Q,VoiceNo,V,depth);
}

// get random byte from LFSR
// source: 0x6e8c
uint8_t Q_VoiceLfoGetRandom(Q_State *Q)
{
    return Q_GetRandom(&Q->LFSR2);
}

// update LFO depth/frequency slides
// source: 0x6eb4
uint16_t Q_VoiceLfoUpdateEnv(uint16_t a,uint16_t b,uint16_t delta)
{
    int32_t d = a-b;
    if(d > 0)
    {
        d -= delta;
        if(d<0)
            d=0;
    }
    else if(d < 0)
    {
        d += delta;
        if(d>0)
            d=0;
    }
    return b+d;
}

// set LFO frequency modulation
// source: 0x6f0a
void Q_VoiceLfoSetMod(Q_State *Q,int VoiceNo,Q_Voice *V,int8_t offset)
{
    int16_t depth = (offset * (V->LfoDepth>>8))>>2;
    V->LfoMod = depth;
}
