#include <string.h>

#include "../qp.h"
#include "helper.h"
#include "tables.h"
#include "track.h"
#include "voice.h"

void S2X_FMClear(S2X_State *S,S2X_FMVoice *V,int VoiceNo)
{
    memset(V,0,sizeof(S2X_FMVoice));
    V->Flag=0;
    V->VoiceNo=VoiceNo;
    V->Lfo=0;

    //Q_DEBUG("clear FM %02d\n",VoiceNo);

    S2X_OPMWrite(S,VoiceNo,0,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,1,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,2,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,3,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,0,OPM_KEYON,0);
}

void S2X_FMKeyOff(S2X_State *S,S2X_FMVoice *V)
{
    //printf("fm v=%02x key off\n",V->VoiceNo);
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_KEYON,0);
    V->Length=0;
    V->Flag&=~(0x10);
}

void S2X_FMSetIns(S2X_State *S,S2X_FMVoice *V,int InsNo)
{
    int i;
    uint32_t pos = V->BaseAddr + S2X_ReadWord(S,V->BaseAddr+0x06)+(32*InsNo);
    //Q_DEBUG("fm v=%02x set ins %02x at %06x\n",V->VoiceNo,InsNo,pos);

    S2X_FMKeyOff(S,V);
    for(i=0;i<4;i++)
        S2X_OPMWrite(S,V->VoiceNo,i,OPM_OP_D1L_RR,0xff);

    V->InsPtr = pos;
    V->InsNo=InsNo;
    V->Flag|=0x80;
    V->Carrier = S2X_FMConnection[S2X_ReadByte(S,pos)&0x07];
    V->ChipFlags = S2X_ReadByte(S,pos)&0x3f;
    V->InsLfo = S2X_ReadByte(S,pos+3);
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL,V->ChipFlags); // mute while setting parameters
    for(i=1;i<28;i++)
        S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL+(i*8),S2X_ReadByte(S,pos+i));
    for(i=0;i<=V->Carrier;i++)
        S2X_OPMWrite(S,V->VoiceNo,3-i,OPM_OP_TL,0x7f);
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_PMS_AMS,0);
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL,V->ChipFlags|0xC0);
    V->Channel->Vars[S2X_CHN_PAN]=0;

    return S2X_FMSetVol(S,V);
}

// sets volume (Reads TL levels from instuments)
void S2X_FMSetVol(S2X_State *S,S2X_FMVoice *V)
{
    if(~V->Flag&0x80)
        return;
    int i,d;
    for(i=0;i<=V->Carrier;i++)
    {
        d = S2X_ReadByte(S,V->InsPtr+0x0b-i)+V->Volume;
        V->TL[i] = (d>127) ? 127 : d;
    }
}

// writes volume + attenuation
void S2X_FMWriteVol(S2X_State *S,S2X_FMVoice *V,int Attenuation)
{
    int i,d;
    for(i=0;i<=V->Carrier;i++)
    {
        d = V->TL[i]+Attenuation;
        if(d<0) d=0;
        S2X_OPMWrite(S,V->VoiceNo,3-i,OPM_OP_TL,(d>127) ? 127 : d);
        //Q_DEBUG"FM v=%02x write vol %02x on operator %d (Carrier:%d)\n",V->VoiceNo,d,3-i,V->Carrier);
    }
}

// sets LFO parameters (notice that only one unique preset can be enabled simultaneously)
void S2X_FMSetLfo(S2X_State *S,S2X_FMVoice *V,int LfoNo)
{
    uint8_t i;
    if(LfoNo==0xff)
    {
        S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_PMS_AMS,0);
        V->Lfo = 0;
        return;
    }
    if(LfoNo!=S->FMLfo)
    {
        // disable lfo for all other channels
        for(i=0;i<8;i++)
            S2X_OPMWrite(S,i,0,OPM_CH_PMS_AMS,0);

        S->FMLfo = LfoNo;
        // read new preset
        uint32_t pos = V->BaseAddr + S2X_ReadWord(S,V->BaseAddr+0x02)+(5*LfoNo);
        S2X_OPMWrite(S,0,0,OPM_LFO_WAV,S2X_ReadByte(S,pos));
        S2X_OPMWrite(S,0,0,OPM_LFO_FRQ,S2X_ReadByte(S,pos+1));
        S->FMLfoPms = S2X_ReadByte(S,pos+2);
        S->FMLfoAms = S2X_ReadByte(S,pos+3);
        uint8_t i = S2X_ReadByte(S,pos+4);
        S->FMLfoDepthDelta = i ? ((0x80-i)>>1) : 0;
        S->FMLfoDepthDelta *= S->FMLfoDepthDelta;

        //Q_DEBUG("fm v=%02x lfo set %02x = %06x\n",V->VoiceNo,LfoNo,pos);
    }
    V->Lfo=LfoNo;
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_PMS_AMS,V->InsLfo);
}

void S2X_FMCommand(S2X_State *S,S2X_Channel *C,S2X_FMVoice *V)
{
    V->Track = C->Track;
    V->Channel = C;
    V->BaseAddr = V->Track->PositionBase;
    //V->BaseAddr = S->FMBase;

    int i=0;
    uint8_t data;
    uint16_t temp;
    while(C->UpdateMask)
    {
        if(C->UpdateMask & 1)
        {
            data = C->Vars[i];
            switch(i)
            {
            case S2X_CHN_WAV:
                S2X_FMSetIns(S,V,data);
                V->Flag|=0x80;
                break;
            case S2X_CHN_FRQ:
                if(data == 0xff)
                {
                    S2X_FMKeyOff(S,V);
                    break;
                }
                else if(C->Vars[S2X_CHN_LEG])
                    C->Vars[S2X_CHN_LEG]--;
                else
                {
                    S2X_FMKeyOff(S,V);
                    V->Delay = C->Vars[S2X_CHN_DEL];
                    V->Flag |= 0x50;
                }
                V->Key = C->Vars[S2X_CHN_FRQ];
                break;
            case S2X_CHN_VOL:
                temp = (~data&0xff) + (~C->Track->TrackVolume&0xff);
                V->Volume = temp>127 ? 127 : temp;
                //Q_DEBUG("FM v=%02x volume set to %02x (%02x + %02x)\n",V->VoiceNo,V->Volume,~data&0xff,~C->Track->TrackVolume&0xff);
                S2X_FMSetVol(S,V);
                break;
            case S2X_CHN_LFO:
                S2X_FMSetLfo(S,V,data);
                break;
            case S2X_CHN_PAN:
                S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL,V->ChipFlags|data);
                break;
            case S2X_CHN_PTA:
                if(data)
                    data = -data;
                V->Pitch.Portamento= data;
            default:
                break;
            }
        }
        i++;
        C->UpdateMask >>= 1;
    }
}

void S2X_FMUpdateReset(S2X_State *S,S2X_FMVoice *V)
{
    S2X_Channel *C = V->Channel;
    if((~V->Flag & 0x40) || V->Delay)
        return;

    // pitch envelope setup
    V->Pitch.EnvNo = C->Vars[S2X_CHN_PIT];
    if(V->Pitch.EnvNo)
    {
        if(C->Vars[S2X_CHN_ENV])
            Q_DEBUG("FM v=%02x Volume envelope requested (%02x)\n",V->VoiceNo,C->Vars[S2X_CHN_ENV]);
        V->Pitch.EnvDepth = C->Vars[S2X_CHN_PITDEP];
        V->Pitch.EnvSpeed = C->Vars[S2X_CHN_PITRAT];
    }
    V->Pitch.EnvBase = V->BaseAddr + 0x04;
    S2X_VoicePitchEnvSet(S,&V->Pitch);
    V->Pitch.PortaFlag = V->Pitch.Portamento;

    // lfo setup
    if(V->Lfo)
    {
        //Q_DEBUG("FM v=%02x Lfo flag set.\n",V->VoiceNo);
        V->LfoFlag=V->Lfo;
        V->LfoDepthCounter=0;
        S2X_OPMWrite(S,0,0,OPM_LFO_DEP,0);
        S2X_OPMWrite(S,0,0,OPM_LFO_DEP,0x80);
    }

    return S2X_FMWriteVol(S,V,V->Track->Fadeout>>8);
}

void S2X_FMUpdateGate(S2X_State *S,S2X_FMVoice *V)
{
    if(V->Length && !(--V->Length))
        return S2X_FMKeyOff(S,V);
}

void S2X_FMUpdateLfo(S2X_State *S,S2X_FMVoice *V)
{
    int d;
    if(!V->LfoFlag)
        return;
    if(S->FMLfoDepthDelta)
    {
        V->LfoDepthCounter += S->FMLfoDepthDelta;
        if(V->LfoDepthCounter < 0x8000)
            return;
        else if(V->LfoDepthCounter < 0x10000)
        {
            d = (V->LfoDepthCounter>>8)&0x7f;
            S2X_OPMWrite(S,0,0,OPM_LFO_DEP,0x80|((d>S->FMLfoPms) ? S->FMLfoPms : d));
            S2X_OPMWrite(S,0,0,OPM_LFO_DEP,0x00|((d>S->FMLfoAms) ? S->FMLfoAms : d));
            return;
        }
    }
    //Q_DEBUG("FM v=%02x Lfo PMS/AMS written.\n",V->VoiceNo);
    S2X_OPMWrite(S,0,0,OPM_LFO_DEP,0x80|(S->FMLfoPms&0x7f));
    S2X_OPMWrite(S,0,0,OPM_LFO_DEP,0x00|(S->FMLfoAms&0x7f));
    V->LfoFlag=0;
}

void S2X_FMUpdatePitch(S2X_State *S,S2X_FMVoice *V)
{
    uint16_t pitch = V->Pitch.Value + V->Pitch.EnvMod;
    pitch += (V->Channel->Vars[S2X_CHN_TRS]<<8)|V->Channel->Vars[S2X_CHN_DTN];
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_KEYCODE,S2X_FMKeyCodes[pitch>>8]);
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_KEYFRAC,pitch&0xff);
}

// 0xd18c
void S2X_FMUpdate(S2X_State *S,S2X_FMVoice *V)
{
    if(~V->Flag & 0x80)
        return;
    V->Pitch.Target = V->Key<<8;
    S2X_FMUpdateGate(S,V);
    S2X_FMUpdateReset(S,V);
    S2X_FMUpdateLfo(S,V);
    S2X_VoicePitchUpdate(S,&V->Pitch);
    S2X_FMUpdatePitch(S,V);

    if(V->Flag&0x40 && V->Delay)
    {
        V->Delay--;
        return;
    }
    else if(V->Flag&0x40)
    {
        //printf("fm v=%02x key on\n",V->VoiceNo);
        //S2X_OPMWrite(S,V->VoiceNo,0,OPM_KEYON,0);
        S2X_OPMWrite(S,V->VoiceNo,0,OPM_KEYON,0x78);
        V->Length = V->Channel->Vars[S2X_CHN_GTM];
        V->Flag &= ~(0x40);
    }
}