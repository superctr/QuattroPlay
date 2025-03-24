/*
    C30 WSG sound driver

    Features:
        Uses the C352, which allows for VGM logging
        Music tracks work
        Sound effects sometimes work
    Limitations:
        Uses a hack to handle notes above 85khz
        Noise frequences above 85khz are not supported at all.
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"
#include "wsg.h"
#include "tables.h"

//#define WSG_DBG 1
#ifdef WSG_DBG
#define WSG_DEBUG(...) Q_DEBUG(__VA_ARGS__)
#else
#define WSG_DEBUG(...)
#endif // WSG_DBG

uint16_t S2X_S1WSGReadHeader(S2X_State *S,enum S2X_S1WSGHeaderId id)
{
    int type = S->ConfigFlags & S2X_CFG_WSG_TYPE;
    int offset = S->WSGBase;
    switch(id)
    {
    case S2X_S1WSG_HEADER_TRACKREQ:
        if(type)
            return 0;
        break;
    case S2X_S1WSG_HEADER_TRACKTYPE:
        if(type)
            return offset+0x0a;
    default:
        if(type)
            id-=S2X_S1WSG_HEADER_TRACK;
        offset += id;
        break;
    }
    return S->WSGBase+S2X_ReadWord(S,offset);
}

void S2X_S1WSGLoadWave(S2X_State *S)
{
    uint16_t pos = S2X_S1WSGReadHeader(S,S2X_S1WSG_HEADER_WAVEFORM);
    WSG_DEBUG("loading WSG Wave Tables from %04x\n",pos);

    int i=0;
    while(i<512)
    {
        S->WSGWaveData[i++] = (S2X_ReadByte(S,pos)&0xf0) + 0x80;
        S->WSGWaveData[i++] = (S2X_ReadByte(S,pos++)<<4) + 0x80;
    }
    i=0;
    pos=0;
    while(i<128)    // compressed waves for high freq sounds
    {
        S->WSGWaveData[512+(i++)] = S->WSGWaveData[pos];
        pos+=4;
    }

    S->PCMChip.wave = S->WSGWaveData;
    S->PCMChip.wave_mask = sizeof(S->WSGWaveData)-1;

#ifdef WSG_DBG
    for(i=0;i<512;i++)
        Q_DEBUG("%02x%s",S->WSGWaveData[i],((i+1)&15) ? " " : "\n");
#endif
}

// Return 1 on failure
int S2X_S1WSGTrackStart(S2X_State *S,int TrackNo,S2X_Track *T,int SongNo)
{
    uint8_t id=0;
    uint8_t data = SongNo-1;
    uint16_t idtab = S2X_S1WSGReadHeader(S,S2X_S1WSG_HEADER_TRACKREQ);
    if(idtab)
    {
        while(id<0x80)
        {
            data = S2X_ReadByte(S,idtab+id);
            if(data == 0xff || data == SongNo-1)
                break;
            id++;
        }
    }
    if(data != 0xff)
    {
        T->Position = S2X_ReadWord(S,S2X_S1WSGReadHeader(S,S2X_S1WSG_HEADER_TRACK)+(2*data));
        T->InitFlag = 0;
#ifdef DEBUG
        uint16_t ttype = S2X_S1WSGReadHeader(S,S2X_S1WSG_HEADER_TRACKTYPE);
        Q_DEBUG("WSG track type: %02x\n", S2X_ReadByte(S,ttype+data));
#endif
        return 1;
    }
    return 0;
}

void S2X_S1WSGTrackUpdate(S2X_State *S,int TrackNo,S2X_Track *T)
{
    uint16_t pos;
    uint8_t flag;
    int ch=0;
    if(!T->InitFlag)
    {
        while(ch<8)
        {
            pos = S->WSGBase+S2X_ReadWord(S,S->WSGBase+T->Position);
            T->InitFlag|=1<<ch;
            flag = S2X_ReadByte(S,S->WSGBase+T->Position+2);
            if(flag == 0xd0)
                break;
            S2X_ChannelClear(S,TrackNo,ch);
            S2X_S1WSGChannelStart(S,TrackNo,&T->Channel[ch],ch,pos,flag);
            WSG_DEBUG("WSG trk %02x ch %d init %04x, %02x\n",TrackNo,ch,pos,flag);
            T->Position+=3;
            flag = S2X_ReadByte(S,S->WSGBase+T->Position);
            if(flag == 0xd0) // need to read twice for end marker
                break;
            ch++;
        }
    }

    flag=0;
    for(ch=0;ch<8;ch++)
    {
        if(T->Channel[ch].WSG.Active)
        {
            S2X_S1WSGChannelUpdate(S,TrackNo,&T->Channel[ch],ch);
            flag++;
        }
    }

    // no channels are playing, we can now disable the track
    if(flag==0)
        S2X_TrackDisable(S,TrackNo);
}

void S2X_S1WSGEnvelopeSet(S2X_State *S,struct S2X_WSGEnvelope *E,int EnvNo)
{
    E->No=EnvNo;
    E->Flag=0;
}

void S2X_S1WSGEnvelopeStart(S2X_State *S,struct S2X_WSGEnvelope *E)
{
    if(E->Flag)
        return;
    E->Pos = S->WSGBase+S2X_ReadWord(S,S2X_S1WSGReadHeader(S,S2X_S1WSG_HEADER_ENVELOPE)+E->No*2);
    //WSG_DEBUG("start envelope %02x at %04x\n",E->No,E->Pos);

}

void S2X_S1WSGEnvelopeUpdate(S2X_State *S,struct S2X_WSGEnvelope *E,S2X_Channel *C)
{
    uint8_t d = S2X_ReadByte(S,E->Pos++);
    if(d < 0x10)
        E->Val = d;
    else
    {
        switch(d)
        {
        default:
        case 0x10: // halt envelope
            E->Pos--;
            break;
        case 0x11: // decay
            d = S2X_ReadByte(S,E->Pos);
            if(!E->Val || (--E->Val&0x0f) != d)
                E->Pos--;
            else
                E->Pos++;
            break;
        case 0x12: // sustain until end
            if(E->Val > C->WSG.SeqWait)
                E->Val=C->WSG.SeqWait;
            E->Pos--;
            break;
        case 0x13: // attack
            d = S2X_ReadByte(S,E->Pos);
            if((++E->Val&0x0f) != d)
                E->Pos--;
            else
                E->Pos++;
            break;
        case 0x14: // loop
            E->Flag=0;
            S2X_S1WSGEnvelopeStart(S,E);
            return S2X_S1WSGEnvelopeUpdate(S,E,C);
        case 0x15: // set legato flag (Prevent envelope reset on new note)
            WSG_DEBUG("lgt flag\n");
            E->Flag=1;
            return S2X_S1WSGEnvelopeUpdate(S,E,C);
        case 0x16: // pitch adjust
            d = S2X_ReadByte(S,E->Pos++);
            int32_t pit = (C->WSG.Freq>>8) * (int8_t)d;
            //WSG_DEBUG("%02x %08x %06x\n",d,pit,C->WSG.Freq);
            C->WSG.Freq += pit;
            return S2X_S1WSGEnvelopeUpdate(S,E,C);
        case 0x17:
            d = S2X_ReadByte(S,E->Pos++);
            C->WSG.WaveNo=d;
            return S2X_S1WSGEnvelopeUpdate(S,E,C);
        }
    }
    E->Val &= 0x0f;
    //...
}

void S2X_S1WSGChannelStart(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo,uint16_t pos,uint8_t flag)
{
    S2X_ChannelClear(S,TrackNo,ChannelNo);

    C->WSG.Active=1;
    C->VoiceNo = flag&7;
    C->WSG.PitchNo = flag>>4;
    C->WSG.SeqWait = 0;
    C->WSG.Env[0].Flag = 1;
    C->WSG.Env[0].Flag = 1;
    C->WSG.SeqPos = pos;
    C->WSG.SeqRepeat = 0;
    C->WSG.SeqLoop = 0;

    int temp=100;
    S2X_VoiceSetPriority(S,C->VoiceNo,TrackNo,ChannelNo,temp);
    // check for other tracks
    pos = S2X_VoiceGetPriority(S,C->VoiceNo,NULL,NULL);
    // no higher priority tracks on the voice? if so, allocate
    if(pos == temp)
    {
        S2X_VoiceSetChannel(S,C->VoiceNo,TrackNo,ChannelNo);
        S2X_VoiceClear(S,C->VoiceNo);
    }
}

void S2X_S1WSGChannelStop(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo)
{
    S->ChannelPriority[C->VoiceNo][TrackNo].channel = 0;
    S->ChannelPriority[C->VoiceNo][TrackNo].priority = 0;

    C->WSG.Active = 0;
    if(C->Enabled)
    {
        C->Enabled=0;
        //c->KeyOnType=0;

        S2X_VoiceClearChannel(S,C->VoiceNo);
        S2X_VoiceClear(S,C->VoiceNo);

        // Switch to next track on the voice
        int next_track = 0;
        int next_channel = 0;
        if(S2X_VoiceGetPriority(S,C->VoiceNo,&next_track,&next_channel))
        {
            S2X_VoiceSetChannel(S,C->VoiceNo,next_track,next_channel);
            S2X_VoiceCommand(S,&S->Track[next_track].Channel[next_channel],0,0);
        }
    }
}


void S2X_S1WSGChannelUpdate(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo)
{
    S2X_VoiceCommand(S,C,0,0);

    uint8_t note = S2X_ReadByte(S,C->WSG.SeqPos), d;
    if(note==0xff)
    {
        C->WSG.SeqPos++;
        d = S2X_ReadByte(S,C->WSG.SeqPos++);
        switch(d)
        {
        case 0x00: // set wave
            C->WSG.WaveSet = S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x01: // set both envelopes
            S2X_S1WSGEnvelopeSet(S,&C->WSG.Env[0],S2X_ReadByte(S,C->WSG.SeqPos));
            S2X_S1WSGEnvelopeSet(S,&C->WSG.Env[1],S2X_ReadByte(S,C->WSG.SeqPos++));
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x09: // set right envelope
            S2X_S1WSGEnvelopeSet(S,&C->WSG.Env[0],S2X_ReadByte(S,C->WSG.SeqPos++));
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0a: // set left envelope
            S2X_S1WSGEnvelopeSet(S,&C->WSG.Env[1],S2X_ReadByte(S,C->WSG.SeqPos++));
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x02: // set tempo
            C->WSG.Tempo = S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x04:
            C->WSG.SeqRepeat++;
            if(C->WSG.SeqRepeat != S2X_ReadByte(S,C->WSG.SeqPos++))
            {
                C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            }
            else
            {
                C->WSG.SeqRepeat=0;
                C->WSG.SeqPos+=2;
            }
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x05:
            C->WSG.SeqLoop++;
            if(C->WSG.SeqLoop == S2X_ReadByte(S,C->WSG.SeqPos++))
            {
                C->WSG.SeqLoop=0;
                C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            }
            else
            {
                C->WSG.SeqPos+=2;
            }
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x06:
            C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x07:
            C->WSG.Noise = C352_FLG_NOISE;
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x08:
            C->WSG.Noise = 0;
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0b: // command is ignored for now...
            // apparently used to stop another WSG track (dspirit)
            // Or to request a DAC sample? (splatter)
            WSG_DEBUG("Command 0b = %02x\n",S2X_ReadByte(S,C->WSG.SeqPos));
            C->WSG.SeqPos+= (S->ConfigFlags&S2X_CFG_WSG_CMD0B) ? 2 : 1;
            return S2X_S1WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        default:
            Q_DEBUG("WSG warning: unimplemented command %02x; ",d);
            WSG_DEBUG("stop channel %02x\n",ChannelNo);
            return S2X_S1WSGChannelStop(S,TrackNo,C,ChannelNo);
        case 0x03:
            WSG_DEBUG("WSG stop track %02x (%02x)\n",TrackNo, S2X_ReadByte(S,C->WSG.SeqPos));
            // special case for splatterhouse credit jingle
            if(S->ConfigFlags & S2X_CFG_WSG_CMD0B && (S->SongRequest[TrackNo]&0x1ff) == 1)
            {
                if(ChannelNo==6)
                {
                    S2X_S1WSGTrackStart(S,TrackNo,&S->Track[TrackNo],2);
                    S->SongRequest[TrackNo]++;
                }
                return;
            }
            return S2X_TrackDisable(S,TrackNo);
        }
    }
    if(!C->WSG.SeqWait)
    {
        if(C->WSG.Noise)
        {
            C->WSG.Freq = note<<8;
        }
        else
        {
            uint8_t key = (note>>4)*3;
            uint16_t notetab = S2X_S1WSGReadHeader(S,S2X_S1WSG_HEADER_PITCHTAB);
            notetab = S->WSGBase+S2X_ReadWord(S,notetab+C->WSG.PitchNo*2);
            C->WSG.Freq = S2X_ReadByte(S,key+notetab)<<16;
            C->WSG.Freq |= S2X_ReadByte(S,key+notetab+1)<<8;
            C->WSG.Freq |= S2X_ReadByte(S,key+notetab+2);
            C->WSG.Freq >>= note&0xf;
        }

        WSG_DEBUG("ch %02x Note=%02x, Len=%02x, E1=%02x, E2=%02x, Frq=%06x at %04x\n",
                ChannelNo,
                note,
                S2X_ReadByte(S,C->WSG.SeqPos+1),
                C->WSG.Env[0].No,
                C->WSG.Env[1].No,
                C->WSG.Freq,
                C->WSG.SeqPos);

        d = S2X_ReadByte(S,C->WSG.SeqPos+1); // note length
        C->WSG.SeqWait = C->WSG.Tempo * d;

        S2X_S1WSGEnvelopeStart(S,&C->WSG.Env[0]);
        S2X_S1WSGEnvelopeStart(S,&C->WSG.Env[1]);

        // set active wave
        C->WSG.WaveNo = C->WSG.WaveSet;
    }

    S2X_S1WSGEnvelopeUpdate(S,&C->WSG.Env[0],C);
    S2X_S1WSGEnvelopeUpdate(S,&C->WSG.Env[1],C);

    if(!C->WSG.Noise && note==0xc0)
    {
        C->WSG.Env[0].Val=0;
        C->WSG.Env[1].Val=0;
        C->WSG.Freq=0x10000;
    }

    C->WSG.Freq |= (C->WSG.WaveNo&0x0f)<<16;

    if(!--C->WSG.SeqWait)
        C->WSG.SeqPos+=2;
}



uint16_t S2X_S86WSGReadHeader(S2X_State *S,enum S2X_S86WSGHeaderId id)
{
    return S->WSGBase+S->WSGHeaders[id];
}

void S2X_S86WSGLoadWave(S2X_State *S)
{
    uint16_t pos = S2X_S86WSGReadHeader(S,S2X_S86WSG_HEADER_WAVEFORM);
    WSG_DEBUG("loading WSG Wave Tables from %04x + %04x\n",S->WSGBase,pos);

    int i=0;
    while(i<512)
    {
        S->WSGWaveData[i++] = (S2X_ReadByte(S,pos)&0xf0) + 0x80;
        S->WSGWaveData[i++] = (S2X_ReadByte(S,pos++)<<4) + 0x80;
    }
    i=0;
    pos=0;
    while(i<128)    // compressed waves for high freq sounds
    {
        S->WSGWaveData[512+(i++)] = S->WSGWaveData[pos];
        pos+=4;
    }

    S->PCMChip.wave = S->WSGWaveData;
    S->PCMChip.wave_mask = sizeof(S->WSGWaveData)-1;

#ifdef WSG_DBG
    for(i=0;i<512;i++)
        Q_DEBUG("%02x%s",S->WSGWaveData[i],((i+1)&15) ? " " : "\n");
#endif
}

// Return 1 on failure
int S2X_S86WSGTrackStart(S2X_State *S,int TrackNo,S2X_Track *T,int SongNo)
{
    uint8_t id=0;
    uint8_t data = SongNo-1;
    uint16_t idtab = S2X_S86WSGReadHeader(S,S2X_S86WSG_HEADER_TRACKREQ);
    if(idtab)
    {
        while(id<0x80)
        {
            data = S2X_ReadByte(S,idtab+id);
            if(data == 0xff || data == SongNo-1)
                break;
            id++;
        }
    }
    if(data != 0xff)
    {
        T->Position = S2X_ReadWord(S,S2X_S86WSGReadHeader(S,S2X_S86WSG_HEADER_TRACK)+(2*data));
        T->InitFlag = 0;
        T->Tempo = S2X_ReadByte(S, S2X_S86WSGReadHeader(S,S2X_S86WSG_HEADER_TEMPO) + data);
        return 1;
    }
    return 0;
}

void S2X_S86WSGTrackUpdate(S2X_State *S,int TrackNo,S2X_Track *T)
{
    uint16_t pos;
    uint8_t flag;
    int ch=0;
    if(!T->InitFlag)
    {
        while(ch<8)
        {
            pos = S2X_ReadByte(S,S->WSGBase+T->Position);
            if(pos == 0x11)
                break;
            T->InitFlag|=1<<ch;
            S2X_ChannelClear(S,TrackNo,ch);
            S2X_S86WSGChannelStart(S,TrackNo,&T->Channel[ch],ch,S->WSGBase+T->Position);
            WSG_DEBUG("WSG trk %02x ch %d init %04x\n",TrackNo,ch,S->WSGBase + T->Position);
            if (S->ConfigFlags & S2X_CFG_WSG_TYPE)
                T->Position += 7;
            else
                T->Position += 6;
            ch++;
        }
    }

    flag=0;
    for(ch=0;ch<8;ch++)
    {
        if(T->Channel[ch].WSG.Active)
        {
            flag++;
            if(T->SyncFlag)
            {
                if(T->SyncFlag <= T->Channel[ch].WSG.SyncFlag)
                    T->SyncFlag = T->Channel[ch].WSG.SyncFlag;
                else
                    T->SyncFlag--;
                T->Channel[ch].WSG.Env[0].Val = 0;
                T->Channel[ch].WSG.Env[1].Val = 0;
            }
            else
            {
                T->SyncFlag = T->Channel[ch].WSG.SyncFlag;
                S2X_VoiceCommand(S,&T->Channel[ch],0,0);
                S2X_S86WSGChannelUpdate(S,TrackNo,&T->Channel[ch],ch);
            }
        }
    }

    // no channels are playing, we can now disable the track
    if(flag==0)
        S2X_TrackDisable(S,TrackNo);
}

void S2X_S86WSGEnvelopeSet(S2X_State *S,struct S2X_WSGEnvelope *E,int EnvNo)
{
    E->No=EnvNo;
    E->Flag=0;
}

void S2X_S86WSGEnvelopeStart(S2X_State *S,struct S2X_WSGEnvelope *E)
{
    if(E->Flag)
        return;
    E->Pos = S->WSGBase+S2X_ReadWord(S,S2X_S86WSGReadHeader(S,S2X_S86WSG_HEADER_ENVELOPE)+E->No*2);
    //WSG_DEBUG("start envelope %02x at %04x\n",E->No,E->Pos);

}

void S2X_S86WSGEnvelopeUpdate(S2X_State *S,struct S2X_WSGEnvelope *E,S2X_Channel *C)
{
    if(E->Flag)
        return;
    uint8_t d = S2X_ReadByte(S,E->Pos++);
    if(d < 0x10)
        E->Val = d;
    else
    {
        switch(d & 0x1e)
        {
        default:
        case 0x10: // halt envelope
            E->Pos--;
            break;
        case 0x12: // sustain until end
            if(E->Val > C->WSG.SeqWait)
                E->Val=C->WSG.SeqWait;
            E->Pos--;
            break;
        case 0x14: // loop
            E->Flag=0;
            S2X_S86WSGEnvelopeStart(S,E);
            return S2X_S86WSGEnvelopeUpdate(S,E,C);
        case 0x16: // decay
            d = S2X_ReadByte(S,E->Pos);
            if(!E->Val || (--E->Val&0x0f) != d)
                E->Pos--;
            else
                E->Pos++;
            break;
        case 0x18: // set wave number
            d = S2X_ReadByte(S,E->Pos++);
            C->WSG.WaveNo = d;
            return S2X_S86WSGEnvelopeUpdate(S,E,C);
        case 0x1a: // add wave number
            d = S2X_ReadByte(S,E->Pos++);
            C->WSG.WaveNo += d;
            return S2X_S86WSGEnvelopeUpdate(S,E,C);
        case 0x1c: // pitch adjust
            d = S2X_ReadByte(S,E->Pos++);
            C->WSG.Freq += ((C->WSG.Freq & 0xfffff) >>8) * (uint8_t)d;
            return S2X_S86WSGEnvelopeUpdate(S,E,C);
        case 0x1e: // pitch adjust
            d = S2X_ReadByte(S,E->Pos++);
            C->WSG.Freq -= ((C->WSG.Freq & 0xfffff) >>8) * (uint8_t)d;
            return S2X_S86WSGEnvelopeUpdate(S,E,C);
        }
    }
    E->Val &= 0x0f;
    if(E->Val > C->WSG.Volume)
        E->Val = C->WSG.Volume;
}

void S2X_S86WSGChannelStart(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo,uint16_t pos)
{
    S2X_ChannelClear(S,TrackNo,ChannelNo);

    C->WSG.Active=1;

    C->WSG.SeqPos = S->WSGBase + S2X_ReadWord(S,pos);
    pos += 2;

    C->VoiceNo = S2X_ReadByte(S,pos++);
    C->WSG.PitchNo = S2X_ReadByte(S,pos++);
    C->WSG.SeqWait = 0;
    C->WSG.WaveNo = S2X_ReadByte(S,pos++);
    //C->WSG.Noise = (C->WSG.WaveNo & 1) ? C352_FLG_NOISE : 0;
    C->WSG.WaveNo &= 0xf0;
    C->WSG.WaveSet = C->WSG.WaveNo;

    C->WSG.Env[0].No = S2X_ReadByte(S,pos++);
    C->WSG.Env[0].Flag = 1;
    C->WSG.SeqRepeat = 0;
    C->WSG.SeqLoop = 0;
    C->WSG.Return = C->WSG.SeqPos;

    C->WSG.Tempo = 2;

    if (S->ConfigFlags & S2X_CFG_WSG_TYPE)
        C->WSG.Volume = S2X_ReadByte(S,pos++);
    else
        C->WSG.Volume = 15;

    WSG_DEBUG("WSG ch %02x Init pos=%04x v=%02x trs=%02x w=%02x env=%02x vol=%02x\n",
            ChannelNo,
            C->WSG.SeqPos,
            C->VoiceNo,
            C->WSG.PitchNo,
            C->WSG.WaveNo,
            C->WSG.Env[0].No,
            C->WSG.Volume);

    int temp=100;
    S2X_VoiceSetPriority(S,C->VoiceNo,TrackNo,ChannelNo,temp);
    // check for other tracks
    pos = S2X_VoiceGetPriority(S,C->VoiceNo,NULL,NULL);
    // no higher priority tracks on the voice? if so, allocate
    if(pos == temp)
    {
        S2X_VoiceSetChannel(S,C->VoiceNo,TrackNo,ChannelNo);
        S2X_VoiceClear(S,C->VoiceNo);
    }
}

void S2X_S86WSGChannelStop(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo)
{
    S->ChannelPriority[C->VoiceNo][TrackNo].channel = 0;
    S->ChannelPriority[C->VoiceNo][TrackNo].priority = 0;

    C->WSG.Active = 0;
    if(C->Enabled)
    {
        C->Enabled=0;
        //c->KeyOnType=0;

        S2X_VoiceClearChannel(S,C->VoiceNo);
        S2X_VoiceClear(S,C->VoiceNo);

        // Switch to next track on the voice
        int next_track = 0;
        int next_channel = 0;
        if(S2X_VoiceGetPriority(S,C->VoiceNo,&next_track,&next_channel))
        {
            S2X_VoiceSetChannel(S,C->VoiceNo,next_track,next_channel);
            S2X_VoiceCommand(S,&S->Track[next_track].Channel[next_channel],0,0);
        }
    }
}


void S2X_S86WSGChannelUpdate(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo)
{
    uint8_t note = S2X_ReadByte(S,C->WSG.SeqPos);
    uint8_t rest = 0;
    if(C->WSG.Noise && note == 0x00)
    {
        C->WSG.SeqPos++;
    }

    if(!C->WSG.Noise && (note & 0xf0) == 0xc0)
    {
        rest = 1;
    }
    else if((C->WSG.Noise && note == 0) || (!C->WSG.Noise && note >= 0xe0))
    {
        uint8_t command = S2X_ReadByte(S,C->WSG.SeqPos++) & 0x1f;
        switch(command)
        {
        case 0x00:
            WSG_DEBUG("WSG stop track %02x (%04x)\n",TrackNo, C->WSG.SeqPos - 1);
            return S2X_TrackDisable(S,TrackNo);
        case 0x01: // set wave
            C->WSG.WaveSet = S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x02: // add wave
            C->WSG.WaveSet += S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x03: // set envelope
            S2X_S86WSGEnvelopeSet(S,&C->WSG.Env[0],S2X_ReadByte(S,C->WSG.SeqPos++));
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x04: // add envelopes
            S2X_S86WSGEnvelopeSet(S,&C->WSG.Env[0],C->WSG.Env[0].No + S2X_ReadByte(S,C->WSG.SeqPos++));
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x05: // repeat
            C->WSG.SeqRepeat++;
            if(C->WSG.RepeatCondition)
            {
                C->WSG.SeqRepeat=0;
                C->WSG.SeqPos+=3;
            }
            else if(C->WSG.SeqRepeat == S2X_ReadByte(S,C->WSG.SeqPos++))
            {
                C->WSG.SeqRepeat=0;
                C->WSG.SeqPos+=2;
            }
            else
            {
                C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            }
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x06: // repeat condition
            C->WSG.RepeatCondition = S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x07: // loop 1
            C->WSG.SeqLoop++;
            if(C->WSG.SeqLoop == S2X_ReadByte(S,C->WSG.SeqPos++))
            {
                //C->WSG.SeqLoop=0;
                C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            }
            else
            {
                C->WSG.SeqPos+=2;
            }
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x08: // loop 2
            C->WSG.SeqLoop2++;
            if(C->WSG.SeqLoop2 == S2X_ReadByte(S,C->WSG.SeqPos++))
            {
                C->WSG.SeqLoop2=0;
                C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            }
            else
            {
                C->WSG.SeqPos+=2;
            }
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x09: // jump
            C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0a: // noise enable
            C->WSG.Noise = C352_FLG_NOISE;
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0b: // noise disable
            C->WSG.Noise = 0;
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0c: // rest (in noise mode)
            rest = 1;
            C->WSG.SeqPos--;
            break;
        case 0x0d: // transpose/detune
            C->WSG.PitchNo = S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0e: // transpose/detune
            C->WSG.PitchNo += S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x0f: // set sync flag
            C->WSG.SyncFlag = S2X_ReadByte(S,C->WSG.SeqPos++);
            //S->Track[TrackNo].SyncFlag = C->WSG.SyncFlag;
            Q_DEBUG("chn %02x sync flag = %02x\n",ChannelNo,C->WSG.SyncFlag);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x10: // clear sync flag
            C->WSG.SyncFlag = 0;
            //S->Track[TrackNo].SyncFlag = 0;
            Q_DEBUG("chn %02x clear sync flag\n",ChannelNo);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x11: // set tempo
            S->Track[TrackNo].Tempo = S2X_ReadByte(S,C->WSG.SeqPos++);
            WSG_DEBUG("tempo = %02x\n",S->Track[TrackNo].Tempo);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x12: // request music (command is ignored for now...)
            Q_DEBUG("WSG command F2 = %02x\n",S2X_ReadByte(S,C->WSG.SeqPos));
            C->WSG.SeqPos += 1;
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x13: // jump
            C->WSG.Return = C->WSG.SeqPos += 2;
            C->WSG.SeqPos = S->WSGBase+S2X_ReadWord(S,C->WSG.SeqPos);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x14: // return
            C->WSG.SeqPos = C->WSG.Return;
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x15: // set volume cap
            C->WSG.Volume = S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        case 0x16: // add volume cap
            C->WSG.Volume += S2X_ReadByte(S,C->WSG.SeqPos++);
            return S2X_S86WSGChannelUpdate(S,TrackNo,C,ChannelNo);
        default:
            Q_DEBUG("WSG warning: unimplemented command %02x; ",command);
            WSG_DEBUG("stop channel %02x\n",ChannelNo);
            return S2X_S86WSGChannelStop(S,TrackNo,C,ChannelNo);

        }
    }
    if(!C->WSG.SeqWait)
    {
        if(rest)
        {
            C->WSG.Env[0].Flag = 1;
            C->WSG.Env[0].Val = 0;
            C->WSG.Freq = 0x10000;
            WSG_DEBUG("WSG rest ");
        }
        else if(C->WSG.Noise)
        {
            C->WSG.Env[0].Flag = 0;
            C->WSG.Freq = note << 9;
            WSG_DEBUG("WSG noise ");
        }
        else
        {
            C->WSG.Env[0].Flag = 0;
            C->WSG.Freq = S2X_S86WSGPitchTable[(note >> 4) + (C->WSG.PitchNo & 15)];

            uint8_t detune = C->WSG.PitchNo >> 4;
            int32_t pit = (C->WSG.Freq >> 8) * detune;
            C->WSG.Freq += pit;

            C->WSG.Freq >>= (note&0xf)-1;
            WSG_DEBUG("WSG note ");
        }

        WSG_DEBUG("ch %02x Tempo=%02x, Trs=%02x, Note=%02x, Len=%02x, Env=%02x, Frq=%06x at %04x\n",
                ChannelNo,
                S->Track[TrackNo].Tempo,
                C->WSG.PitchNo,
                note,
                S2X_ReadByte(S,C->WSG.SeqPos+1),
                C->WSG.Env[0].No,
                C->WSG.Freq,
                C->WSG.SeqPos);

        C->WSG.Tempo = S->Track[TrackNo].Tempo;
        note = S2X_ReadByte(S,C->WSG.SeqPos+1); // note length

        C->WSG.SeqWait = C->WSG.Tempo * note;

        S2X_S86WSGEnvelopeStart(S,&C->WSG.Env[0]);
    }

    S2X_S86WSGEnvelopeUpdate(S,&C->WSG.Env[0],C);

    C->WSG.Env[1].Val = C->WSG.Env[0].Val;

    //C->WSG.Freq |= (C->WSG.WaveSet & 0xf0) << 16;
    //C->WSG.WaveNo = C->WSG.Freq >> 16;
    C->WSG.WaveNo = C->WSG.WaveSet & 0xf0;

    if(!--C->WSG.SeqWait)
        C->WSG.SeqPos+=2;
}

