#include <string.h>

#include "s2x.h"
#include "helper.h"
#include "tables.h"
#include "track.h"
#include "voice.h"

#define OLD_VOL_MODE (S->ConfigFlags & S2X_CFG_FM_VOL)
#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)
#define SYSTEM86 (S->DriverType == S2X_TYPE_SYSTEM86)

void S2X_FMClear(S2X_State *S,S2X_FMVoice *V,int VoiceNo)
{
    int trk=V->TrackNo,chn=V->ChannelNo;
    memset(V,0,sizeof(S2X_FMVoice));

    V->TrackNo=trk;
    V->ChannelNo=chn;

    V->Flag=0;
    V->VoiceNo=VoiceNo;
    V->Lfo=0;
    V->Pitch.FM = V;

    S2X_OPMWrite(S,VoiceNo,0,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,1,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,2,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,3,OPM_OP_TL,127);
    S2X_OPMWrite(S,VoiceNo,0,OPM_KEYON,0);
}

void S2X_FMKeyOff(S2X_State *S,S2X_FMVoice *V)
{
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_KEYON,0);
    V->Length=0;
    V->Flag&=~(0x10);
}

// there is no check to see if the current instrument has already been loaded
// so some games set it twice in a row and you will see "OPM is not keeping up"
// that is normal...
void S2X_FMSetIns(S2X_State *S,S2X_FMVoice *V,int InsNo)
{
    int i;
    uint32_t pos;

    if(SYSTEM86)
        pos = V->BaseAddr+S->FMInsTab;
    else
        pos = (V->BaseAddr&0xffff00)+S2X_ReadWord(S,V->BaseAddr+0x06);
    pos += (32*InsNo);

    S2X_FMKeyOff(S,V);
    for(i=0;i<4;i++)
        S2X_OPMWrite(S,V->VoiceNo,i,OPM_OP_D1L_RR,0xff);

    V->InsPtr = pos;
    V->InsNo=InsNo;
    V->Flag|=0x80;
    V->Carrier = SYSTEM86 ? 0 : S2X_FMConnection[S2X_ReadByte(S,pos)&0x07];
    V->ChipFlags = S2X_ReadByte(S,pos)&0x3f;
    V->InsLfo = S2X_ReadByte(S,pos+3);
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL,V->ChipFlags); // mute while setting parameters
    for(i=2;i<28;i++)
        S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL+(i*8),S2X_ReadByte(S,pos+i));
    if(!SYSTEM86)
    {
        for(i=0;i<=V->Carrier;i++)
            S2X_OPMWrite(S,V->VoiceNo,3-i,OPM_OP_TL,0x7f);
        S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_PMS_AMS,0);
    }
    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_CONTROL,V->ChipFlags|0xC0);
    V->Channel->Vars[S2X_CHN_PAN]=0xc0;

    if(!SYSTEM86)
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
        if(SYSTEM86)
        {
            d = (~(V->Volume>>1)) & 0x7f;
        }
        else if(OLD_VOL_MODE)
        {
            d = ~(((S2X_ReadByte(S,V->InsPtr+0x0b-i)&0x7f)^0x7f)*V->Volume) >> 8;
            d &= 0x7f;
        }
        else
        {
            d = S2X_ReadByte(S,V->InsPtr+0x0b-i)+V->Volume;
        }
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
        uint32_t pos = V->BaseAddr&0xffff00;
        pos += S2X_ReadWord(S,V->BaseAddr+0x02)+(5*LfoNo);

        S->FMLfoWav = S2X_ReadByte(S,pos);
        S->FMLfoFrq = S2X_ReadByte(S,pos+1);
        S2X_OPMWrite(S,0,0,OPM_LFO_WAV,S->FMLfoWav);
        S2X_OPMWrite(S,0,0,OPM_LFO_FRQ,S->FMLfoFrq);

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
    if(SYSTEM1 && !SYSTEM86)
        V->BaseAddr+=2;
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
                else if(!C->Vars[S2X_CHN_LEG])
                {
                    S2X_FMKeyOff(S,V);
                    if(!SYSTEM1)
                        V->Delay = C->Vars[S2X_CHN_DEL];
                    V->Flag |= 0x50;
                }
                V->Key = C->Vars[S2X_CHN_FRQ];
                break;
            case S2X_CHN_DEL:
                if(SYSTEM1)
                    V->Delay = data;
                break;
            case S2X_CHN_VOL:
                if(OLD_VOL_MODE)
                {
                    V->Volume = (data*C->Track->TrackVolume)>>8;
                }
                else
                {
                    temp = (~data&0xff) + (~C->Track->TrackVolume&0xff);
                    V->Volume = temp>127 ? 127 : temp;
                }
                S2X_FMSetVol(S,V);
                if(SYSTEM86)
                    S2X_FMWriteVol(S,V,V->Track->Fadeout>>8);
                break;
            case S2X_CHN_LFO:
                if(SYSTEM86)
                    S2X_OPMWrite(S,V->VoiceNo,0,OPM_CH_PMS_AMS,data&0x73);
                else
                    S2X_FMSetLfo(S,V,data);
                break;
            case S2X_CHN_PAN:
                if(S->ConfigFlags & S2X_CFG_FM_PAN)
                    data = ((data&0x40)<<1)|((data&0x80)>>1);
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
        V->Pitch.VolDepth = C->Vars[S2X_CHN_ENV];
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
    if(!SYSTEM86)
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
    if(!V->LfoFlag || SYSTEM86)
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
            //Q_DEBUG("Pms=%02x Ams=%02x d=%02x\n",S->FMLfoPms,S->FMLfoAms,d);
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
        S2X_OPMWrite(S,V->VoiceNo,0,OPM_KEYON,0x78);
        V->Length = V->Channel->Vars[S2X_CHN_GTM];
        V->Flag &= ~(0x40);
    }
}
