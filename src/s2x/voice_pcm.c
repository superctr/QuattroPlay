#include <string.h>

#include "../qp.h"
#include "helper.h"
#include "tables.h"
#include "track.h"
#include "voice.h"

void S2X_PCMClear(S2X_State *S,S2X_PCMVoice *V,int VoiceNo)
{
    memset(V,0,sizeof(S2X_PCMVoice));
    V->Flag=0;
    V->VoiceNo=VoiceNo;

    Q_DEBUG("initialize ch %02d\n",VoiceNo);

    S2X_C352_W(S,VoiceNo,C352_FLAGS,0);
}
void S2X_PCMCommand(S2X_State *S,S2X_Channel *C,S2X_PCMVoice *V)
{
    V->Track = C->Track;
    V->Channel = C;

    int i=0;
    uint8_t data;
    while(C->UpdateMask)
    {
        if(C->UpdateMask & 1)
        {
            data = C->Vars[i];
            switch(i)
            {
            case S2X_CHN_WAV:
                //Q_DEBUG("ch %02d wav set %02x\n",V->VoiceNo,C->Vars[S2X_CHN_WAV]);
                break;
            case S2X_CHN_FRQ: // freq set / key on
                if(data == 0xff)
                {
                    //Q_DEBUG("ch %02d key off\n",V->VoiceNo);
                    V->Length=1;
                    break;
                }
                else if(C->Vars[S2X_CHN_LEG])
                    C->Vars[S2X_CHN_LEG]--;
                else
                {
                    //Q_DEBUG("ch %02d key on\n",V->VoiceNo);
                    V->Delay = C->Vars[S2X_CHN_DEL];
                    V->Flag |= 0x40;
                }
                V->Key = C->Vars[S2X_CHN_FRQ];
                break;
            case S2X_CHN_ENV: // envelope
                V->EnvPtr = S->PCMBase+S2X_ReadWord(S,S->PCMBase+S2X_ReadWord(S,S->PCMBase+0x0a)+(2*data));
                //Q_DEBUG("ch %02d env set %02x = %06x\n",V->VoiceNo,data,V->EnvPtr);
                break;
            case S2X_CHN_VOL: // volume
                V->Volume = (data*C->Track->TrackVolume)>>8;
                break;
            case S2X_CHN_PAN:
                V->Pan = data;
                V->PanSlide=0;
                break;
            case S2X_CHN_PANENV:
                V->PanSlidePtr = S->PCMBase+S2X_ReadWord(S,S->PCMBase+0x0c)+(4*data);
                //Q_DEBUG("V=%02d pan slide set  %06x\n",V->VoiceNo,V->PanSlidePtr);
                V->PanSlide = S2X_ReadByte(S,V->PanSlidePtr+3);
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

// 0xdb72
void S2X_PCMUpdateReset(S2X_State *S,S2X_PCMVoice *V)
{
    uint8_t temp;
    S2X_Channel *C = V->Channel;
    if((~V->Flag & 0x40) || V->Delay)
        return;

    // pitch envelope setup
    V->Pitch.EnvNo = C->Vars[S2X_CHN_PIT];
    if(V->Pitch.EnvNo)
    {
        V->Pitch.EnvDepth = C->Vars[S2X_CHN_PITDEP];
        V->Pitch.EnvSpeed = C->Vars[S2X_CHN_PITRAT];
    }
    V->Pitch.EnvBase = S->PCMBase + 0x08;
    S2X_VoicePitchEnvSet(S,&V->Pitch);
    V->Pitch.PortaFlag = V->Pitch.Portamento;

    // envelope setup
    V->EnvPos=V->EnvPtr;
    temp = S2X_ReadByte(S,V->EnvPos++);
    V->EnvDelta  = S2X_EnvelopeRateTable[temp&0x7f];
    V->EnvTarget = S2X_ReadByte(S,V->EnvPos++)<<8;
    V->EnvValue  = temp&0x80 ? V->EnvValue : 0;

    // pan slide setup
    V->PanSlideSpeed = V->PanSlide;
    if(V->PanSlide)
    {
        //Q_DEBUG("V=%02d pan slide read %06x\n",V->VoiceNo,V->PanSlidePtr);

        V->PanSlideDelay = S2X_ReadByte(S,V->PanSlidePtr);
        if(V->PanSlideDelay != 0xfe)
            V->Pan = S2X_ReadByte(S,V->PanSlidePtr+1);
    }
}

// 0xdcce
void S2X_PCMPanSlideUpdate(S2X_State* S,S2X_PCMVoice *V)
{
    int pan = V->Pan;

    if(!V->PanSlideSpeed)
        return;
    if(V->PanSlideDelay >= 0xfe)
    {
        pan += (int8_t)V->PanSlide;
        if(pan > 0xff)
        {
            pan = 0xff;
            V->PanSlide = -V->PanSlide;
        }
        else if(pan < 0)
        {
            pan = 0;
            V->PanSlide = -V->PanSlide;
        }
    }
    else if(V->PanSlideDelay)
    {
        V->PanSlideDelay--;
        return;
    }
    else
    {
        int8_t delta = V->PanSlideSpeed;
        uint8_t target = S2X_ReadByte(S,V->PanSlidePtr+2);
        if((delta > 0 && (pan+delta > target)) || (delta < 0 && (pan+delta < target)))
        {
            V->PanSlideSpeed=0;
            pan = target;
        }
        else
            pan += delta;


    }
    V->Pan = pan;
}

// 0xdda5
void S2X_PCMPanUpdate(S2X_State* S,S2X_PCMVoice *V)
{
    uint8_t v=(V->Volume),e=(V->EnvValue>>8);
    uint16_t vol,left,right;
    vol=(v*e);
    vol-=V->Track->Fadeout;
    if(vol<0)
        vol=0;
    left=right=vol&0xff00;
    if(V->Pan < 0x80)
        right = (right>>8) * (V->Pan&0x7f);
    else if(V->Pan > 0x80)
        left = (left>>8) * (-V->Pan&0x7f);

    //printf("Ch %02d L=%04x, R=%04x, Vl=%02x, Ev=%02x, Fv=%04x\n",V->VoiceNo,left,right,v,e,vol);
    S2X_C352_W(S,V->VoiceNo,C352_VOL_FRONT,(left&0xff00)|(right>>8));
}


void S2X_PCMEnvelopeAdvance(S2X_State* S,S2X_PCMVoice *V)
{
    //if(V->VoiceNo < 8)
    //    Q_DEBUG("V=%02d env adv %06x\n",V->VoiceNo,V->EnvPos);

    uint8_t d = S2X_ReadByte(S,V->EnvPos);
    uint8_t t = S2X_ReadByte(S,V->EnvPos+1);

    V->EnvPos+=2;
    V->EnvDelta = S2X_EnvelopeRateTable[d&0x7f];
    V->EnvTarget = t<<8;

    return S2X_PCMPanUpdate(S,V);
}

// 0xdd91
void S2X_PCMEnvelopeCommand(S2X_State* S,S2X_PCMVoice *V)
{
    //uint8_t d,t;
    V->EnvValue = V->EnvTarget;
    uint8_t d = S2X_ReadByte(S,V->EnvPos);
    uint8_t t = S2X_ReadByte(S,V->EnvPos+1);

    if(!d && (t>0x80))
    {
        //Q_DEBUG("V=%02d env end (0)\n",V->VoiceNo);
        S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);
        V->Flag &= 0x7f;
        return;
    }
    else if(!d && (t==0x80))
    {
        return S2X_PCMPanUpdate(S,V);
    }
    else if(!d)
    {
        V->EnvPos = V->EnvPtr + (t<<1); // ?
    }

    return S2X_PCMEnvelopeAdvance(S,V);
}

void S2X_PCMEnvelopeUpdate(S2X_State* S,S2X_PCMVoice *V)
{
    uint32_t pos = V->EnvPos;
    uint8_t d;
    int32_t a;

    if(!(pos&0xffff))
        return;
    if(V->Length && !(--V->Length))
    {
        do {
            d=S2X_ReadByte(S,V->EnvPos);
            //t=S2X_ReadByte(S,V->EnvPos++);
            V->EnvPos+=2;
            //Q_DEBUG("%06x=%02x ",V->EnvPos-2,d);
            //V->EnvPos++;
        }
        while (d);
        d = S2X_ReadByte(S,V->EnvPos-1);
        if(d==0xff)
        {
            //Q_DEBUG("V=%02d key off (1)\n",V->VoiceNo);
            S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);
            V->Flag &= 0x7f;
            return;
        }
        //return S2X_PCMEnvelopeAdvance(S,V);

        return S2X_PCMEnvelopeAdvance(S,V);

    }
    else
    {
        //if(V->VoiceNo==0)
        //    printf("%04x, %04x, %04x\n",V->EnvValue,V->EnvTarget,V->EnvDelta);
        d = V->EnvValue>>8;
        //t = V->EnvTarget>>8;
        if(V->EnvValue<V->EnvTarget)
        {
            a=V->EnvValue+V->EnvDelta;
            V->EnvValue=a;
            if((a>0xffff) || (a>V->EnvTarget))
                return S2X_PCMEnvelopeCommand(S,V);
        }
        else if(V->EnvValue>V->EnvTarget)
        {
            a=V->EnvValue-V->EnvDelta;
            V->EnvValue=a;
            if((a<0) || (a<V->EnvTarget))
                return S2X_PCMEnvelopeCommand(S,V);
        }
    }
    return S2X_PCMPanUpdate(S,V);
}

void S2X_PCMWaveUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    if(V->Channel->Vars[S2X_CHN_WAV] == V->WaveNo)
        return;
    V->WaveNo = V->Channel->Vars[S2X_CHN_WAV];
    uint32_t pos = S->PCMBase+S2X_ReadWord(S,S->PCMBase+0x02)+(10*V->WaveNo);

    S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);

    V->WaveBank = S2X_ReadByte(S,pos++);
    V->WaveFlag = S2X_ReadByte(S,pos++);
    S2X_C352_W(S,V->VoiceNo,C352_WAVE_START,S2X_ReadWord(S,pos));
    S2X_C352_W(S,V->VoiceNo,C352_WAVE_END,S2X_ReadWord(S,pos+2));
    S2X_C352_W(S,V->VoiceNo,C352_WAVE_LOOP,S2X_ReadWord(S,pos+4)+1);
    V->WavePitch = S2X_ReadWord(S,pos+6);

    V->ChipFlag=0;
    if(V->WaveFlag & 0x10)
        V->ChipFlag |= C352_FLG_LOOP;
    if(V->WaveFlag & 0x08)
        V->ChipFlag |= C352_FLG_MULAW;
#if 0
    Q_DEBUG("ch %02d wave %02x (Pos: %06x, B:%02x F:%02x S:%04x E:%04X L:%04x P:%04x)\n",
            V->VoiceNo,
            V->WaveNo,
            pos,
            V->WaveBank,
            V->WaveFlag,
            S2X_ReadWord(S,pos),
            S2X_ReadWord(S,pos+2),
            S2X_ReadWord(S,pos+4),
            V->WavePitch);
#endif
}
void S2X_PCMPitchUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    uint16_t freq1,freq2,reg,temp;

    uint32_t pitch = V->Pitch.Value + V->Pitch.EnvMod;
    pitch += (V->Channel->Vars[S2X_CHN_TRS]<<8)|V->Channel->Vars[S2X_CHN_DTN];

    pitch &= 0x7fff;

    freq1 = S2X_PitchTable[pitch>>8];
    freq2 = S2X_PitchTable[(pitch>>8)+1];
    temp = freq1 + (((uint16_t)(freq2-freq1)*(pitch&0xff))>>8);

    reg = (temp*V->WavePitch)>>8;

    //if(V->VoiceNo == 5)
    //    Q_DEBUG("p=%04x,%04x,%04x,%04x,%04x (W=%04x)\n",pitch,freq1,freq2,temp,reg,V->WavePitch);
    S2X_C352_W(S,V->VoiceNo,C352_FREQUENCY,reg>>1);
}

// 0xdada
void S2X_PCMUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    if(!V->Flag)
        return;
    V->Pitch.Target = V->Key<<8;
    //V->Pitch.Portamento = V->Channel->Vars[S2X_CHN_PTA];

    S2X_PCMUpdateReset(S,V);
    S2X_PCMEnvelopeUpdate(S,V);
    S2X_PCMPanSlideUpdate(S,V);
    S2X_VoicePitchUpdate(S,&V->Pitch);

    if(V->Flag & 0x40)
    {
        //Q_DEBUG("ch %02d key on executed\n",V->VoiceNo);
        if(V->Delay)
        {
            V->Delay--;
            return;
        }
        S2X_PCMWaveUpdate(S,V);
        S2X_PCMPitchUpdate(S,V);
        S2X_C352_W(S,V->VoiceNo,C352_WAVE_BANK,V->WaveBank);
        S2X_C352_W(S,V->VoiceNo,C352_FLAGS,V->ChipFlag|C352_FLG_KEYON);
        V->Length = V->Channel->Vars[S2X_CHN_GTM];
        V->Flag<<=1;
    }
    else
    {
        S2X_PCMPitchUpdate(S,V);
    }
}

void S2X_PlayPercussion(S2X_State *S,int VoiceNo,int WaveNo,int VolMod)
{
    uint32_t pos = S->PCMBase+S2X_ReadWord(S,S->PCMBase+0x04)+(10*WaveNo);

    uint8_t bank = S2X_ReadByte(S,pos);
    uint8_t flag = S2X_ReadByte(S,pos+1);
    int16_t left = S2X_ReadByte(S,pos+3)-VolMod;
    int16_t right= S2X_ReadByte(S,pos+2)-VolMod;
    if(left < 0) left=0;
    if(right < 0) right=0;

    S2X_C352_W(S,VoiceNo,C352_FLAGS,0);
    uint16_t ChipFlag;

    ChipFlag=0;
    //if(flag & 0x10)
    //    ChipFlag |= C352_FLG_LOOP;
    if(flag & 0x08)
        ChipFlag |= C352_FLG_MULAW;

    S2X_C352_W(S,VoiceNo,C352_VOL_FRONT,(left<<8)|(right&0xff));
    S2X_C352_W(S,VoiceNo,C352_FREQUENCY,S2X_ReadWord(S,pos+4)>>1);
    S2X_C352_W(S,VoiceNo,C352_WAVE_START,S2X_ReadWord(S,pos+6));
    S2X_C352_W(S,VoiceNo,C352_WAVE_END,S2X_ReadWord(S,pos+8));
    S2X_C352_W(S,VoiceNo,C352_WAVE_BANK,bank);
    S2X_C352_W(S,VoiceNo,C352_FLAGS,ChipFlag|C352_FLG_KEYON);
#if 0
    Q_DEBUG("ch %02d perc %02x %06x Vol:%04x Freq=%04x Start=%04x End=%04x Bank=%04x Flag=%04x\n",VoiceNo,WaveNo,pos,
            S2X_C352_R(S,VoiceNo,C352_VOL_FRONT),
            S2X_C352_R(S,VoiceNo,C352_FREQUENCY),
            S2X_C352_R(S,VoiceNo,C352_WAVE_START),
            S2X_C352_R(S,VoiceNo,C352_WAVE_END),
            S2X_C352_R(S,VoiceNo,C352_WAVE_BANK),
            S2X_C352_R(S,VoiceNo,C352_FLAGS));
#endif

}

