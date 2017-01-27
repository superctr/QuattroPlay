#include <string.h>

#include "s2x.h"
#include "helper.h"
#include "tables.h"
#include "track.h"
#include "voice.h"
#include "../drv/tables.h" /* quattro pitch table (used for NA-1/NA-2) */

#define ADSR_ENV (S->ConfigFlags & (S2X_CFG_PCM_ADSR|S2X_CFG_PCM_NEWADSR))
#define NEW_ADSR (S->ConfigFlags & S2X_CFG_PCM_NEWADSR)
#define PAN_INVERT (S->ConfigFlags & S2X_CFG_PCM_PAN)

#define SYSTEMNA (S->DriverType == S2X_TYPE_NA)

void S2X_PCMClear(S2X_State *S,S2X_PCMVoice *V,int VoiceNo)
{
    int trk=V->TrackNo,chn=V->ChannelNo;
    memset(V,0,sizeof(S2X_PCMVoice));

    V->TrackNo=trk;
    V->ChannelNo=chn;

    V->Flag=0;
    V->VoiceNo=VoiceNo;

    V->Pan = 0x80;
    V->WaveNo = 0xff;
    V->Pitch.FM = 0;

    S2X_C352_W(S,VoiceNo,C352_FLAGS,0);
}
void S2X_PCMKeyOff(S2X_State* S,S2X_PCMVoice *V)
{
    S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);
    V->Flag &= 0x6f;
}
void S2X_PCMCommand(S2X_State *S,S2X_Channel *C,S2X_PCMVoice *V)
{
    V->Track = C->Track;
    V->Channel = C;
    V->BaseAddr = V->Track->PositionBase;

    int i=0;
    uint8_t data;
    while(C->UpdateMask)
    {
        if(C->UpdateMask & 1)
        {
            data = C->Vars[i];
            switch(i)
            {
            case S2X_CHN_FRQ: // freq set / key on
                if(data == 0xff)
                {
                    V->Flag &= ~(0x10);
                    V->Length=1;
                    break;
                }
                else if(!C->Vars[S2X_CHN_LEG])
                {
                    V->Delay = C->Vars[S2X_CHN_DEL];
                    V->Flag |= 0x50;
                }
                V->Key = C->Vars[S2X_CHN_FRQ];
                break;
            case S2X_CHN_ENV: // envelope
                V->EnvPtr = V->BaseAddr+S2X_ReadWord(S,V->BaseAddr+S2X_ReadWord(S,V->BaseAddr+0x0a)+(data*2));
                S2X_PCMAdsrSet(S,V,data);
                break;
            case S2X_CHN_VOL: // volume
                V->Volume = (data*C->Track->TrackVolume)>>8;
                break;
            case S2X_CHN_PAN:
                V->Pan = data;
                V->PanSlide=0;
                break;
            case S2X_CHN_PANENV:
                V->PanSlidePtr = V->BaseAddr+S2X_ReadWord(S,V->BaseAddr+0x0c)+(4*data);
                V->PanSlide = S2X_ReadByte(S,V->PanSlidePtr+3);
                break;
            case S2X_CHN_PTA:
                if(data)
                    data = -data;
                V->Pitch.Portamento= data;
                break;
            case S2X_CHN_C18:
                Q_DEBUG("ch %02d enable link effect, param: %02d \n",V->VoiceNo,data);
            default:
                break;
            }
        }
        i++;
        C->UpdateMask >>= 1;
    }
}

static void S2X_PCMAdsrSetVal(S2X_State *S,uint8_t val,uint8_t *v1,uint8_t *v2)
{
    if(!NEW_ADSR)
    {
        *v1 = val;
        *v2 = 0;
        return;
    }
    else if(val<0xf8)
    {
        *v1 = val>>3;
        *v2 = S2X_AdsrTable[val&7];
    }
    else
    {
        *v1 = S2X_AdsrTable[val^0xf7];
        *v2 = 0;
    }
}
void S2X_PCMAdsrSet(S2X_State *S,S2X_PCMVoice *V,uint8_t EnvNo)
{
    uint32_t pos = V->BaseAddr+S2X_ReadWord(S,V->BaseAddr+0x0a)+(EnvNo*4);
    S2X_PCMAdsrSetVal(S,S2X_ReadByte(S,pos+0),&V->EnvAttack,&V->EnvAttackFine);
    S2X_PCMAdsrSetVal(S,S2X_ReadByte(S,pos+1),&V->EnvDecay,&V->EnvDecayFine);
    S2X_PCMAdsrSetVal(S,S2X_ReadByte(S,pos+3),&V->EnvRelease,&V->EnvReleaseFine);
    V->EnvSustain = S2X_ReadByte(S,pos+2);
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
    V->Pitch.EnvBase = V->BaseAddr + 0x08;
    S2X_VoicePitchEnvSet(S,&V->Pitch);
    V->Pitch.PortaFlag = V->Pitch.Portamento;

    // envelope setup
    V->EnvPos=V->EnvPtr;

    if(ADSR_ENV)
    {
        V->EnvTarget=1;
    }
    else
    {
        temp = S2X_ReadByte(S,V->EnvPos++);
        V->EnvDelta  = S2X_EnvelopeRateTable[temp&0x7f];
        V->EnvTarget = S2X_ReadByte(S,V->EnvPos++)<<8;
        V->EnvValue  = temp&0x80 ? V->EnvValue : 0;
    }
    V->Length=0;

    // pan slide setup
    V->PanSlideSpeed = V->PanSlide;
    if(V->PanSlide)
    {
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

    uint16_t left,right;
    int32_t vol;
    vol=(v*e);
    vol-=V->Track->Fadeout;
    if(vol<0)
        vol=0;
    left=right=vol&0xff00;
    if(V->Pan < 0x80)
        right = (right>>7) * (V->Pan&0x7f);
    else if(V->Pan > 0x80)
        left = (left>>7) * (-V->Pan&0x7f);

    if(PAN_INVERT)
        S2X_C352_W(S,V->VoiceNo,C352_VOL_FRONT,(right&0xff00)|(left>>8));
    else
        S2X_C352_W(S,V->VoiceNo,C352_VOL_FRONT,(left&0xff00)|(right>>8));
}


#define UPDATE_FINE(_v_) if(NEW_ADSR) _v_=(_v_<<1)|(_v_>>7);
// updates the ADSR envelope
void S2X_PCMAdsrUpdate(S2X_State* S, S2X_PCMVoice *V)
{
    if(V->Length && !(--V->Length))
        V->EnvTarget=4;

    int d = V->EnvValue>>8;

    uint8_t atk = V->EnvAttack + (V->EnvAttackFine>>7);
    uint8_t dec = V->EnvDecay + (V->EnvDecayFine>>7);
    uint8_t sus = V->EnvSustain;
    uint8_t rel = V->EnvRelease + (V->EnvReleaseFine>>7);

    switch(V->EnvTarget&7)
    {
    case 0:
        return;
    case 1: // attack
        d+=atk;
        if(d>0xff)
        {
            d=0xff;
            V->EnvTarget++;
        }
        UPDATE_FINE(V->EnvAttackFine);
        break;
    case 2: // decay
        d-=dec;
        if(d<=sus)
        {
            d=sus;
            V->EnvTarget++;
        }
        UPDATE_FINE(V->EnvDecayFine);
        break;
    case 4: // release
        V->EnvTarget^=8;
        if((V->EnvTarget&8) || NEW_ADSR)
        {
            d-=rel;
            if(d<0)
            {
                V->EnvValue=V->EnvTarget=0;
                S2X_PCMKeyOff(S,V);
                return;
            }
        }
        UPDATE_FINE(V->EnvReleaseFine);
    default:
        break;
    }
    V->EnvValue=d<<8;
    return S2X_PCMPanUpdate(S,V);
}

void S2X_PCMEnvelopeAdvance(S2X_State* S,S2X_PCMVoice *V)
{
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
        S2X_PCMKeyOff(S,V);
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

// updates the volume table envelope
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
            V->EnvPos+=2;
        }
        while (d);
        d = S2X_ReadByte(S,V->EnvPos-1);
        if(d==0xff)
        {
            S2X_PCMKeyOff(S,V);
            return;
        }
        return S2X_PCMEnvelopeAdvance(S,V);
    }
    else
    {
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

// NA-1/NA-2 sample offsets are in words, while C352 sample offsets are in bytes
// so we use the link mode to ensure that it loops back to the correct bank
void S2X_PCMLinkUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    if(!V->LinkMode)
        return;
    if(V->LinkMode == 1)
    {
        S2X_C352_W(S,V->VoiceNo,C352_WAVE_START,V->WaveBank);
        V->LinkMode++;
        return;
    }
    // TODO: may need another link mode in case we find samples > 65535 bytes...
}

void S2X_PCMWaveUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    if(V->Channel->Vars[S2X_CHN_WAV] == V->WaveNo && !V->LinkMode)
        return;
    V->WaveNo = V->Channel->Vars[S2X_CHN_WAV];
    uint32_t pos = V->BaseAddr+S2X_ReadWord(S,V->BaseAddr+0x02)+(10*V->WaveNo);

    V->WaveBank = S2X_ReadByte(S,pos++);
    V->WaveFlag = S2X_ReadByte(S,pos++);

    uint32_t start = S2X_ReadWord(S,pos);
    uint32_t end = S2X_ReadWord(S,pos+2);
    uint32_t loop = S2X_ReadWord(S,pos+4)+1;

    if(SYSTEMNA)
    {
        loop-=1;
        end-=1;
        V->WaveBank = S->WaveBank[V->VoiceNo/4] + (start>>15);
        start = ((start&0x7fff)<<1) + S->WaveBase[S->BankSelect][V->WaveBank];
        end = ((end&0xffff)<<1) + S->WaveBase[S->BankSelect][V->WaveBank] + 1;
        loop = ((loop&0xffff)<<1) + S->WaveBase[S->BankSelect][V->WaveBank];
        V->WaveBank = start>>16;
    }

    S2X_C352_W(S,V->VoiceNo,C352_WAVE_START,start);
    S2X_C352_W(S,V->VoiceNo,C352_WAVE_END,end);
    S2X_C352_W(S,V->VoiceNo,C352_WAVE_LOOP,loop);

    V->WavePitch = S2X_ReadWord(S,pos+6);

    V->LinkMode = 0;
    V->ChipFlag=0;
    if(V->WaveFlag & 0x10)
        V->ChipFlag |= C352_FLG_LOOP;
    if(SYSTEMNA)
    {
        if(V->ChipFlag & C352_FLG_LOOP && (end^loop)&0x10000)
        {
            V->LinkMode=1;
            V->ChipFlag |= C352_FLG_LINK;
        }
        if(V->WaveFlag & 0x01)
            V->ChipFlag |= C352_FLG_MULAW;
        if(V->WaveFlag & 0x04)
            V->ChipFlag |= C352_FLG_NOISE;
        if(V->WaveFlag & 0x40)
            V->ChipFlag |= C352_FLG_PHASEFL|C352_FLG_PHASEFR;
        if(V->WaveFlag & 0x08)
            V->ChipFlag ^= C352_FLG_PHASEFL;
    }
    else
    {
        if(V->WaveFlag & 0x08)
            V->ChipFlag |= C352_FLG_MULAW;
    }

#if 0
    Q_DEBUG("ch %02x wave %02x (Pos: %06x, B:%02x F:%02x S:%04x E:%04X L:%04x P:%04x, lm=%d)\n",
            V->VoiceNo,
            V->WaveNo,
            pos,
            V->WaveBank,
            V->WaveFlag,
            start, //S2X_ReadWord(S,pos),
            end, //S2X_ReadWord(S,pos+2),
            loop, //S2X_ReadWord(S,pos+4),
            V->WavePitch,
            V->LinkMode);
#endif
}
void S2X_PCMPitchUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    uint16_t freq1,freq2,reg,temp;

    uint16_t pitch = V->Pitch.Value + V->Pitch.EnvMod;
    pitch += (V->Channel->Vars[S2X_CHN_TRS]<<8)|V->Channel->Vars[S2X_CHN_DTN];

    if(!SYSTEMNA)
    {
        pitch &= 0x7fff;
        freq1 = S2X_PitchTable[pitch>>8];
        freq2 = S2X_PitchTable[(pitch>>8)+1];
        temp = freq1 + (((uint16_t)(freq2-freq1)*(pitch&0xff))>>8);

        reg = (temp*V->WavePitch)>>8;
        S2X_C352_W(S,V->VoiceNo,C352_FREQUENCY,reg>>1);
    }
    else
    {
        // we use the pitch table from quattro for now...
        pitch = (pitch+V->WavePitch+0x900)&0x7fff;

        if(pitch>0x6b00)
            pitch=0x6b00;
        freq1 = Q_PitchTable[pitch>>8];
        freq2 = Q_PitchTable[(pitch>>8)+1];
        freq1 += ((uint16_t)(freq2-freq1)*(pitch&0xff))>>8;

        S2X_C352_W(S,V->VoiceNo,C352_FREQUENCY,freq1);
    }
}

// 0xdada
void S2X_PCMUpdate(S2X_State *S,S2X_PCMVoice *V)
{
    if(!(V->Flag&0xc0))
        return;
    V->Pitch.Target = V->Key<<8;
    //V->Pitch.Portamento = V->Channel->Vars[S2X_CHN_PTA];

    S2X_PCMLinkUpdate(S,V);
    S2X_PCMUpdateReset(S,V);
    if(ADSR_ENV)
        S2X_PCMAdsrUpdate(S,V);
    else
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
        S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);
        S2X_PCMWaveUpdate(S,V);
        S2X_PCMPitchUpdate(S,V);
        S2X_C352_W(S,V->VoiceNo,C352_WAVE_BANK,V->WaveBank);
        S2X_C352_W(S,V->VoiceNo,C352_FLAGS,V->ChipFlag|C352_FLG_KEYON);
        V->Length = V->Channel->Vars[S2X_CHN_GTM];
        V->Flag=((V->Flag&0xbf)|0x80);
    }
    else
    {
        S2X_PCMPitchUpdate(S,V);
    }
}

void S2X_PlayPercussion(S2X_State *S,int VoiceNo,int BaseAddr,int WaveNo,int VolMod)
{
    uint32_t pos = BaseAddr+S2X_ReadWord(S,BaseAddr+0x04)+(10*WaveNo);

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
    if(SYSTEMNA)
    {
        if(flag & 0x01)
            ChipFlag |= C352_FLG_MULAW;
        if(flag & 0x04)
            ChipFlag |= C352_FLG_NOISE;
        if(flag & 0x40)
            ChipFlag |= C352_FLG_PHASEFL|C352_FLG_PHASEFR;
        if(flag & 0x08)
            ChipFlag ^= C352_FLG_PHASEFL;
    }
    else
    {
        if(flag & 0x08)
            ChipFlag |= C352_FLG_MULAW;
    }

    if(PAN_INVERT)
        S2X_C352_W(S,VoiceNo,C352_VOL_FRONT,(right<<8)|(left&0xff));
    else
        S2X_C352_W(S,VoiceNo,C352_VOL_FRONT,(left<<8)|(right&0xff));
    //S2X_C352_W(S,VoiceNo,C352_FREQUENCY,S2X_ReadWord(S,pos+4)>>1);

    uint32_t start = S2X_ReadWord(S,pos+6);
    uint32_t end = S2X_ReadWord(S,pos+8);
    if(SYSTEMNA)
    {
        bank = S->WaveBank[VoiceNo/4] + (start>>15);
        start = ((start&0x7fff)<<1) + S->WaveBase[S->BankSelect][bank];
        end = ((end&0x7fff)<<1) + S->WaveBase[S->BankSelect][bank];
        bank = start>>16;

        S2X_C352_W(S,VoiceNo,C352_FREQUENCY,S2X_ReadWord(S,pos+4));
    }
    else
    {
        S2X_C352_W(S,VoiceNo,C352_FREQUENCY,S2X_ReadWord(S,pos+4)>>1);
    }
    S2X_C352_W(S,VoiceNo,C352_WAVE_START,start);
    S2X_C352_W(S,VoiceNo,C352_WAVE_END,end);
    //S2X_C352_W(S,VoiceNo,C352_WAVE_START,S2X_ReadWord(S,pos+6));
    //S2X_C352_W(S,VoiceNo,C352_WAVE_END,S2X_ReadWord(S,pos+8));
    S2X_C352_W(S,VoiceNo,C352_WAVE_BANK,bank);

    S2X_C352_W(S,VoiceNo,C352_FLAGS,ChipFlag|C352_FLG_KEYON);

    S->SE[VoiceNo&7].Type = 1;
    S->SE[VoiceNo&7].Wave = WaveNo;
    S->SE[VoiceNo&7].Voice = VoiceNo;
#if 0
    Q_DEBUG("ch %02x perc %02x %06x Vol:%04x Freq=%04x Start=%04x End=%04x Bank=%04x Flag=%04x\n",VoiceNo,WaveNo,pos,
            S2X_C352_R(S,VoiceNo,C352_VOL_FRONT),
            S2X_C352_R(S,VoiceNo,C352_FREQUENCY),
            S2X_C352_R(S,VoiceNo,C352_WAVE_START),
            S2X_C352_R(S,VoiceNo,C352_WAVE_END),
            S2X_C352_R(S,VoiceNo,C352_WAVE_BANK),
            S2X_C352_R(S,VoiceNo,C352_FLAGS));
#endif

}

