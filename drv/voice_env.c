/*
    Quattro - volume envelope functions
*/
#include "quattro.h"
#include "voice.h"
#include "helper.h"
#include "tables.h"

// reset envelope (split from Q_VoiceKeyOn)
// source: 0x6530
void Q_VoiceEnvSet(Q_State *Q,int VoiceNo,Q_Voice* V)
{
    if(V->EnvNo)
    {
        V->EnvIndexPos = Q->TableOffset[Q_TOFFSET_ENVTABLE]+(2*(V->EnvNo-1));
        V->EnvPos = Q->McuDataPosBase | Q_ReadWord(Q,V->EnvIndexPos);

        V->GateTimeLeft = V->GateTime;

        if(Q_ReadByte(Q,V->EnvPos) < 0x80)
            V->EnvValue = 0xffff;

        Q_VoiceEnvRead(Q,VoiceNo,V);
    }
    else
    {
        V->EnvState = Q_ENV_DISABLE;
        V->EnvValue = 0;
    }
}

// read byte from envelope and update state
// source: 0x76fc
void Q_VoiceEnvRead(Q_State *Q,int VoiceNo,Q_Voice* V)
{

    uint8_t d = Q_ReadByte(Q,V->EnvPos++);
    uint8_t target, value;

    switch(d)
    {
    case 0xff:
        //printf("V=%02x, envelope end\n",VoiceNo);
        Q_VoiceDisable(Q,VoiceNo,V);
        break;
    case 0xfe:
        V->EnvState = Q_ENV_SUSTAIN;
        V->EnvPos--; // we expect to read the sustain byte later
        break;
    case 0x00:
        d = Q_ReadByte(Q,V->EnvPos);
        V->EnvPos = Q->McuDataPosBase | Q_ReadWord(Q,V->EnvIndexPos + (2*d));
        d = Q_ReadByte(Q,V->EnvPos++); // read new delta
    default:
        target = Q_ReadByte(Q,V->EnvPos++);
        value = V->EnvValue>>8;
        V->EnvTarget = target<<8;
        if(value == target)
            return Q_VoiceEnvRead(Q,VoiceNo,V); // values match, read next entry
        else if(value > target)
            V->EnvState = Q_ENV_ATTACK;
        else
            V->EnvState = Q_ENV_DECAY;
        break;
    }
    V->EnvDelta = Q_EnvelopeRateTable[d];
}

// call 0x1a - envelope update function
// source:  0x7664
// vectors: 0x7672 (decay), 0x7696 (attack), 0x76b2 (sustain)
void Q_VoiceEnvUpdate(Q_State *Q,int VoiceNo,Q_Voice *V)
{
    int32_t vol = V->EnvValue;
    uint8_t d;
    switch(V->EnvState)
    {
    case Q_ENV_DISABLE:
        return;
    default:
        break;
    case Q_ENV_ATTACK:
    case Q_ENV_DECAY:
        vol = V->EnvState == Q_ENV_ATTACK ? vol - V->EnvDelta
                                          : vol + V->EnvDelta;
        if(vol > 0xffff || vol < 0)
            break;
        else if(V->EnvState == Q_ENV_ATTACK && vol <= V->EnvTarget)
            break;
        else if(V->EnvState == Q_ENV_DECAY && vol >= V->EnvTarget)
            break;
        V->EnvValue = vol;
    case Q_ENV_SUSTAIN:
        if(V->GateTimeLeft == 0)
            return;
        if(--V->GateTimeLeft > 0)
            return;
        do
        {
            d = Q_ReadByte(Q,V->EnvPos++);

            if(d==0xff)
            {
                return Q_VoiceDisable(Q,VoiceNo,V);
            }
            else if(d!=0xfe)
            {
                V->EnvPos++;
            }
        } while(d != 0xfe);
        break;
    }

    V->EnvValue = V->EnvTarget|0x80;
    return Q_VoiceEnvRead(Q,VoiceNo,V);
}

