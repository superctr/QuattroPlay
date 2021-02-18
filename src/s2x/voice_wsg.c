#include <string.h>

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"

void S2X_WSGClear(S2X_State *S,S2X_WSGVoice *V,int VoiceNo)
{
    int trk=V->TrackNo,chn=V->ChannelNo;
    memset(V,0,sizeof(S2X_WSGVoice));

    V->TrackNo=trk;
    V->ChannelNo=chn;

    V->WaveNo=0xff;
    V->LastWaveNo=0xff;
    V->LastPitch=0;

    V->VoiceNo=VoiceNo;

    S2X_C352_W(S,VoiceNo,C352_FLAGS,0);
    S2X_C352_W(S,V->VoiceNo,C352_VOL_FRONT,0);
    S2X_C352_W(S,V->VoiceNo,C352_VOL_REAR,0);
}

void S2X_WSGCommand(S2X_State *S,S2X_Channel *C,S2X_WSGVoice *V)
{
    V->Track = C->Track;
    V->Channel = C;
}

void S2X_WSGUpdate(S2X_State *S,S2X_WSGVoice *V)
{
    if(!V->Channel || !V->Channel->WSG.Active)
    {
        if(S2X_C352_R(S,V->VoiceNo,C352_FLAGS))
            S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);
        return;
    }

    S2X_WSGChannel *W = &V->Channel->WSG;

    V->Pitch = ((W->Freq & 0xfffff) * 0x24) >> 7;
    if(V->Pitch > 0xffff)
        S2X_C352_W(S,V->VoiceNo,C352_FREQUENCY,V->Pitch/4);
    else
        S2X_C352_W(S,V->VoiceNo,C352_FREQUENCY,V->Pitch);

    V->WaveNo = W->WaveNo>>4;
    if(V->WaveNo != V->LastWaveNo || (V->Pitch^V->LastPitch)&0xffff0000 )
    {
        S2X_C352_W(S,V->VoiceNo,C352_FLAGS,0);
        S2X_C352_W(S,V->VoiceNo,C352_WAVE_BANK,0);

        if(V->Pitch>0xffff) // hack to allow pitch > 85khz (C352 max freq)
        {
            S2X_C352_W(S,V->VoiceNo,C352_WAVE_START,512+(V->WaveNo<<3));
            S2X_C352_W(S,V->VoiceNo,C352_WAVE_LOOP,512+(V->WaveNo<<3));
            S2X_C352_W(S,V->VoiceNo,C352_WAVE_END,519+(V->WaveNo<<3));
        }
        else
        {
            S2X_C352_W(S,V->VoiceNo,C352_WAVE_START,V->WaveNo<<5);
            S2X_C352_W(S,V->VoiceNo,C352_WAVE_LOOP,V->WaveNo<<5);
            S2X_C352_W(S,V->VoiceNo,C352_WAVE_END,31+(V->WaveNo<<5));
        }
        S2X_C352_W(S,V->VoiceNo,C352_FLAGS,C352_FLG_KEYON|C352_FLG_FILTER|C352_FLG_LOOP|W->Noise);
        V->LastWaveNo = V->WaveNo;
    }

    V->LastPitch = V->Pitch;

    uint16_t vol = (W->Env[1].Val * 8)<<8;
    vol |= W->Env[0].Val * 8;
    S2X_C352_W(S,V->VoiceNo,C352_VOL_FRONT,vol);
}

