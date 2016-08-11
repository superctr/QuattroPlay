/*
    Quattro - voice functions
*/
#include "quattro.h"
#include "voice.h"
#include "wave.h"
#include "helper.h"
#include "tables.h"

// Call 0x08 - Allocate a voice to a channel and disables previous channel on voice.
// source: 0x4dd6 (call 0x14 at 0x4e78 has a similar function)
void Q_VoiceSetChannel(Q_State *Q,int VoiceNo,int TrackNo,int ChannelNo)
{
    //printf("Trk %02x.%02x, Allocate voice %02x\n",TrackNo,ChannelNo,VoiceNo);

    Q_Channel* new_ch = &Q->Track[TrackNo].Channel[ChannelNo];
    Q_Channel* old_ch = Q->ActiveChannel[VoiceNo];

    if(old_ch != NULL && old_ch != new_ch)
        old_ch->Enabled=0;
    new_ch->Enabled=0xff;

    Q->ActiveChannel[VoiceNo] = new_ch;
    Q->Voice[VoiceNo].ChannelNo = ChannelNo;
    Q->Voice[VoiceNo].TrackNo = TrackNo+1;

}

// Call 0x12 - Clears voice allocation on channel.
// source: 0x4e16
void Q_VoiceClearChannel(Q_State *Q,int VoiceNo)
{
    Q->ActiveChannel[VoiceNo] = NULL;
    Q->Voice[VoiceNo].TrackNo = 0;
}

// Call 0x0e - find highest priority for the voice
// source: 0x4e44
uint16_t Q_VoiceGetPriority(Q_State *Q,int VoiceNo,int* TrackNo,int* ChannelNo)
{
    int i;
    int track = 0;
    int channel=0;
    int priority=0;
    for(i=0;i<Q->TrackCount;i++)
    {
        if(Q->ChannelPriority[VoiceNo][i].priority > priority)
        {
            track=i;
            channel=Q->ChannelPriority[VoiceNo][i].channel;
            priority=Q->ChannelPriority[VoiceNo][i].priority;
        }
    }

    if(TrackNo != NULL)
        *TrackNo = track;
    if(ChannelNo != NULL)
        *ChannelNo = channel;

    return priority;
}

// Call 0x10 - sets priority for a channel on specified voice
// source: 0x4d88
void Q_VoiceSetPriority(Q_State *Q,int VoiceNo,int TrackNo,int ChannelNo,int Priority)
{
    Q->ChannelPriority[VoiceNo][TrackNo].channel = ChannelNo;
    Q->ChannelPriority[VoiceNo][TrackNo].priority = Priority;
}


// Call 0x24 - process an event
// source: 0x6340
void Q_VoiceProcessEvent(Q_State *Q,int VoiceNo,Q_Voice *V,Q_VoiceEvent *E)
{
    Q_Channel *C = E->Channel;
    uint8_t mode;
    //Q_Voice *V = &Q->Voice[VoiceNo];
    //printf("V=%02x event t:%04x, m:%02x d:%02x v:%04x \n",VoiceNo,E->Time,E->Mode,E->Value,*E->Volume);

    V->Channel = E->Channel;
    V->TrackVol = E->Volume;

    mode = E->Mode&0x7f;

    if(mode == Q_EVENTMODE_NOTE && E->Value == 0x7f)
    {
        // key off
        if(V->EnvNo)
        {
            // finish envelope
            V->GateTimeLeft = 1;
            // flag key off for display purposes
            if(V->Enabled)
                V->Enabled = 2;
        }
        else
        {
            // no envelope - cutoff
            V->Enabled = 0;
            Q_C352_W(Q,VoiceNo,C352_FLAGS,0);
        }
        return;
    }

    V->EnvNo      = C->EnvNo;
    V->PitchEnvNo = C->PitchEnvNo;
    V->Volume     = C->Volume;
    V->Pan        = C->Pan;
    V->Detune     = C->Detune;
    V->BaseNote   = C->BaseNote;

    // spaghetti switch ...
    switch(mode)
    {
    default:
    case Q_EVENTMODE_NOTE:
        if(E->Value > 0x7f)
        {
            Q_WaveSet(Q,VoiceNo,V,E->Value&0x7f);
            break;
        }
        V->BaseNote = E->Value;
    case Q_EVENTMODE_VOL:
    case Q_EVENTMODE_PAN:
        if(mode == Q_EVENTMODE_VOL)
        {
            V->Volume = E->Value;
        }
        else if(mode == Q_EVENTMODE_PAN)
        {
            V->Pan = E->Value;
            C->PanMode = Q_PANMODE_IMM;
        }
    case Q_EVENTMODE_OFFSET:
        V->SampleOffset = C->SampleOffset;

        if(C->WaveNo & 0x8000 || C->WaveNo != V->WaveNo)
        {
            C->WaveNo &= 0x7fff;
            Q_WaveSet(Q,VoiceNo,V,C->WaveNo);
        }
        else if(V->WaveLinkFlag && !(E->Mode&Q_EVENTMODE_LEGATO))
        {
            Q_WaveReset(Q,VoiceNo,V);
        }
        break;
    }

    V->NoteDelay = C->NoteDelay;
    V->GateTime = C->GateTime;
    V->SampleOffset = C->SampleOffset;
    V->Transpose = C->Transpose;
    V->LfoNo = C->LfoNo;
    V->Portamento = C->Portamento;
    V->PanMode = C->PanMode;
    V->PitchReg = C->PitchReg;

    V->BaseNote += V->Transpose;
    V->Enabled=1;

    if(~E->Mode & Q_EVENTMODE_LEGATO)
    {
        Q_VoiceKeyOn(Q,VoiceNo,V);
    }
}

// Call 0x18 - executes key on
// source: 0x649c
void Q_VoiceKeyOn(Q_State *Q,int VoiceNo,Q_Voice* V)
{
    //C352_Voice *CV = &Q->Chip.v[VoiceNo];
    //CV->flags = 0;
    Q_C352_W(Q,VoiceNo,C352_FLAGS,0);

    Q_VoicePitchEnvSet(Q,VoiceNo,V);

    Q_VoiceEnvSet(Q,VoiceNo,V);

    V->PitchEnvMod = 0;
    V->LfoMod = 0;

    Q_VoicePanSet(Q,VoiceNo,V);
    Q_VoiceLfoSet(Q,VoiceNo,V);

    //CV->wave_bank = V->WaveBank;
    //CV->flags = V->WaveFlags |= C352_FLG_KEYON;
    Q_C352_W(Q,VoiceNo,C352_WAVE_BANK,V->WaveBank);
    Q_C352_W(Q,VoiceNo,C352_FLAGS,    V->WaveFlags|C352_FLG_KEYON);

}

// Call 0x0c - voice update
// source: 0x6aae
void Q_VoiceUpdate(Q_State *Q,int VoiceNo,Q_Voice* V)
{
    //C352_Voice *CV = &Q->Chip.v[VoiceNo];
    uint16_t flags = Q_C352_R(Q,VoiceNo,C352_FLAGS);
    uint16_t pitch;
    uint16_t freq1, freq2, vol;

    if(flags & C352_FLG_BUSY)
    {
        if(flags & C352_FLG_LINK)
            Q_WaveLinkUpdate(Q,VoiceNo,V);
    }
    else if(~flags & C352_FLG_KEYON)
    {
        //printf("V=%02x sample terminated\n",VoiceNo);
        V->EnvState = Q_ENV_DISABLE;
        V->Enabled=0;
        return;
    }

    if(V->PitchReg)
    {
        V->BaseNote = Q->Register[V->PitchReg]>>8;
        V->Detune = Q->Register[V->PitchReg]&0xff;
    }

    Q_VoiceEnvUpdate(Q,VoiceNo,V);
    Q_VoicePitchEnvUpdate(Q,VoiceNo,V);
    V->PitchTarget = (V->BaseNote<<8) | V->Detune;

    if(V->Portamento)
        Q_VoicePortaUpdate(Q,VoiceNo,V);
    else
        V->Pitch = V->PitchTarget;
    Q_VoiceLfoUpdate(Q,VoiceNo,V);

    // calculate pitch
    pitch = V->Pitch+V->PitchEnvMod+V->LfoMod+Q->BasePitch;
    pitch += V->WaveTranspose;

    // get frequencies from table
    freq1 = Q_PitchTable[pitch>>8];
    freq2 = Q_PitchTable[(pitch>>8)+1];
    freq1 += ((uint16_t)(freq2-freq1)*(pitch&0xff))>>8;

    // overflow for some H8 drivers.
    if(pitch >= 0x6b00 && Q->EnablePitchOverflow)
        freq1 = Q->PitchOverflow;

    Q_C352_W(Q,VoiceNo,C352_FREQUENCY,freq1);
    //CV->freq = freq1;
    V->FreqReg = freq1;

    vol = V->Volume + (V->EnvValue>>8) + (*V->TrackVol);
    if(vol>0xff)
        vol = 0xff;
    V->VolumeMod = vol;

    return Q_VoicePanUpdate(Q,VoiceNo,V);
}
