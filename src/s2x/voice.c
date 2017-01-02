#include "../qp.h"
#include "helper.h"
#include "voice.h"

void S2X_VoiceSetChannel(S2X_State *S,int VoiceNo,int TrackNo,int ChannelNo)
{
    S2X_Channel* new_ch = &S->Track[TrackNo].Channel[ChannelNo];
    S2X_Channel* old_ch = S->ActiveChannel[VoiceNo];

    if(old_ch != NULL && old_ch != new_ch)
        old_ch->Enabled=0;
    new_ch->Enabled=0xff;

    S->ActiveChannel[VoiceNo] = new_ch;
    //S->Voice[VoiceNo].ChannelNo = ChannelNo;
    //S->Voice[VoiceNo].TrackNo = TrackNo+1;
}

void S2X_VoiceClearChannel(S2X_State *S,int VoiceNo)
{
    S->ActiveChannel[VoiceNo] = NULL;
    //S->Voice[VoiceNo].TrackNo = 0;
}

uint16_t S2X_VoiceGetPriority(S2X_State *S,int VoiceNo,int* TrackNo,int* ChannelNo)
{
    int i;
    int track = 0;
    int channel=0;
    int priority=0;
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        if(S->ChannelPriority[VoiceNo][i].priority > priority)
        {
            track=i;
            channel=S->ChannelPriority[VoiceNo][i].channel;
            priority=S->ChannelPriority[VoiceNo][i].priority;
        }
    }

    if(TrackNo != NULL)
        *TrackNo = track;
    if(ChannelNo != NULL)
        *ChannelNo = channel;

    return priority;
}

void S2X_VoiceSetPriority(S2X_State *S,int VoiceNo,int TrackNo,int ChannelNo,int Priority)
{
    S->ChannelPriority[VoiceNo][TrackNo].channel = ChannelNo;
    S->ChannelPriority[VoiceNo][TrackNo].priority = Priority;
}

int S2X_GetVoiceType(S2X_State *S,int VoiceNo)
{
    if(VoiceNo < 16)
        return S2X_VOICE_TYPE_PCM;
    if(VoiceNo < 24)
        return 0; // PCM sound effects, no need to update
    if(VoiceNo < 32)
        return S2X_VOICE_TYPE_FM;
    return 0;
}
int S2X_GetVoiceIndex(S2X_State *S,int VoiceNo,int VoiceType)
{
    switch(VoiceType)
    {
    case S2X_VOICE_TYPE_PCM:
        return VoiceNo; //for now
    case S2X_VOICE_TYPE_FM:
        return VoiceNo&7;
    default:
        return -1;
    }
}


// interface for PCM/FM specific functions
void S2X_VoiceClear(S2X_State *S,int VoiceNo)
{
    int type = S2X_GetVoiceType(S,VoiceNo);
    int index = S2X_GetVoiceIndex(S,VoiceNo,type);
    switch(type)
    {
    default:
        return;
    case S2X_VOICE_TYPE_FM:
        return S2X_FMClear(S,&S->FM[index],index);
    case S2X_VOICE_TYPE_PCM:
        return S2X_PCMClear(S,&S->PCM[index],index);
    }
}
void S2X_VoiceCommand(S2X_State *S,S2X_Channel *C,int Command,uint8_t Data)
{
    if(!C->Enabled)
        return;
    int type = S2X_GetVoiceType(S,C->VoiceNo);
    int index = S2X_GetVoiceIndex(S,C->VoiceNo,type);
    switch(type)
    {
    default:
        return;
    case S2X_VOICE_TYPE_FM:
        return S2X_FMCommand(S,C,&S->FM[index]);
    case S2X_VOICE_TYPE_PCM:
        return S2X_PCMCommand(S,C,&S->PCM[index]);
    }
}
void S2X_VoiceUpdate(S2X_State *S,int VoiceNo)
{
    int type = S2X_GetVoiceType(S,VoiceNo);
    int index = S2X_GetVoiceIndex(S,VoiceNo,type);
    switch(type)
    {
    default:
        return;
    case S2X_VOICE_TYPE_FM:
        return S2X_FMUpdate(S,&S->FM[index]);
    case S2X_VOICE_TYPE_PCM:
        return S2X_PCMUpdate(S,&S->PCM[index]);
    }

}

// pitch envelope support is copied straight from quattro
// should work the same, only real difference is that depth/rate is set separately here

void S2X_VoicePitchEnvSet(S2X_State *S,struct S2X_Pitch *P)
{
    P->EnvMod=0;

    if(!P->EnvNo)
        return;

    P->EnvPos = S2X_ReadWord(S,(P->EnvBase&0xffff00)+S2X_ReadWord(S,P->EnvBase)+(2*(P->EnvNo)));

    P->EnvPos += P->EnvBase&0xffff00;

    P->EnvLoop = P->EnvPos;
    P->EnvData = S2X_ReadByte(S,P->EnvPos++);
    P->EnvCounter=0;
}

void S2X_VoicePitchEnvUpdate(S2X_State *S,struct S2X_Pitch *P)
{
    if(!P->EnvNo)
        return;

    uint16_t counter = P->EnvCounter+P->EnvSpeed;
    uint16_t step;
    int16_t target;
    P->EnvCounter = counter&0xff;
    if(counter>0xff)
        P->EnvData = S2X_ReadByte(S,P->EnvPos++);
    counter&=0xff;

    target = S2X_ReadByte(S,P->EnvPos);
    if(target==0xfd)
    {
        // continue reading next envelope
        P->EnvPos++;
        P->EnvLoop = P->EnvPos;
        return;
    }
    else if(target==0xfe)
    {
        // loop
        P->EnvPos = P->EnvLoop;
        P->EnvCounter=0;
        return;
    }
    else if(target>=0xf0)
    {
        // end
        P->EnvValue=P->EnvData<<8;
        S2X_VoicePitchEnvSetMod(S,P);
        P->EnvNo=0;
        return;
    }

    step = (target-P->EnvData)*counter;
    P->EnvValue = (P->EnvData<<8) + step;
    return S2X_VoicePitchEnvSetMod(S,P);
}

void S2X_VoicePitchEnvSetMod(S2X_State *S,struct S2X_Pitch *P)
{
    uint8_t depth = P->EnvDepth;
    int16_t val = (P->EnvValue-0x6400)>>1; // 100<<8
    int32_t mod = (val*depth)>>8;
    P->EnvMod = mod&0xffff;
}

void S2X_VoicePitchUpdate(S2X_State *S,struct S2X_Pitch *P)
{
    S2X_VoicePitchEnvUpdate(S,P);

    if(P->Value == P->Target)
        return;

    if(!P->PortaFlag)
    {
        P->Value = P->Target;
        return;
    }

    int16_t difference = P->Target - P->Value;
    int16_t step = difference>>8;

    if(difference < 0)
        step--;
    else
        step++;

    P->Value += (step*P->Portamento)/2;

    if(((P->Target-P->Value) ^ difference) < 0)
        P->Value = P->Target;
}
