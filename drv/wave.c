/*
    Quattro - wave (sample) functions
*/
#include "quattro.h"
#include "voice.h"
#include "helper.h"

// reset wave
// source: 0x77c2
void Q_WaveSet(Q_State* Q,int VoiceNo,Q_Voice* V,uint16_t WaveNo)
{
    //printf("write wave no %04x\n",WaveNo);
    V->WaveNo = WaveNo;

    V->WavePos = Q->McuDataPosBase | Q_ReadWord(Q,Q->TableOffset[Q_TOFFSET_WAVETABLE]+(2*WaveNo));
    //printf("V=%02x write wave no %04x at %06x\n",VoiceNo,WaveNo,V->WavePos);

    V->WaveTranspose = Q_ReadWord(Q,V->WavePos);
    V->WaveBank  = Q_ReadWord(Q,V->WavePos+0x02);
    V->WaveFlags = Q_ReadWord(Q,V->WavePos+0x04);
    Q_WaveReset(Q,VoiceNo,V);
}

// write C352 wave position registers and reset link position
// source: 0x780e
void Q_WaveReset(Q_State* Q,int VoiceNo,Q_Voice* V)
{
    //C352_Voice *CV = &Q->Chip.v[VoiceNo];
    //CV->wave_start = Q_ReadWord(Q,V->WavePos+0x06)+V->SampleOffset;
    //CV->wave_end   = Q_ReadWord(Q,V->WavePos+0x08);
    //CV->wave_loop  = Q_ReadWord(Q,V->WavePos+0x0a);

    Q_C352_W(Q,VoiceNo,C352_WAVE_START,Q_ReadWord(Q,V->WavePos+0x06)+V->SampleOffset);
    Q_C352_W(Q,VoiceNo,C352_WAVE_END,  Q_ReadWord(Q,V->WavePos+0x08));
    Q_C352_W(Q,VoiceNo,C352_WAVE_LOOP, Q_ReadWord(Q,V->WavePos+0x0a));

    V->WaveLinkFlag = V->WaveFlags & C352_FLG_LINK ? 1 : 0;
    V->WaveLinkPos = V->WavePos+2;
}

// call 0x2a - update linked wave
// source: 0x6f3a
void Q_WaveLinkUpdate(Q_State* Q,int VoiceNo,Q_Voice *V)
{
    uint16_t cflags = Q_C352_R(Q,VoiceNo,C352_FLAGS);
    //C352_Voice *CV = &Q->Chip.v[VoiceNo];
    uint16_t NextFlag;

    if(V->WaveLinkFlag == 1)
    {
        if(Q_ReadWord(Q,V->WaveLinkPos+0x0a) & C352_FLG_REVERSE)
            V->EnvNo++;

        Q_C352_W(Q,VoiceNo,C352_WAVE_START,Q_ReadWord(Q,V->WaveLinkPos+0x08)); // next bank
        Q_C352_W(Q,VoiceNo,C352_WAVE_LOOP, Q_ReadWord(Q,V->WaveLinkPos+0x0c)); // start position of next link

        V->WaveLinkPos += 8;
        V->WaveLinkFlag++;
    }
    else if(cflags & C352_FLG_LOOPHIST)
    {
        Q_C352_W(Q,VoiceNo,C352_WAVE_END,Q_ReadWord(Q,V->WaveLinkPos+0x06)); // end position of this link
        NextFlag = Q_ReadWord(Q,V->WaveLinkPos+0x02);
        if(NextFlag & C352_FLG_LINK)
        {
            V->WaveLinkPos += 8;
            // jump (link loop)
            if(Q_ReadWord(Q,V->WaveLinkPos)&0x8000)
                V->WaveLinkPos = (V->WaveLinkPos&0xff0000) | Q_ReadWord(Q,V->WaveLinkPos+2);

            Q_C352_W(Q,VoiceNo,C352_FLAGS,     (NextFlag&0xfffe)|(Q_ReadWord(Q,V->WaveLinkPos+0x02)&0x01)); // ?
            Q_C352_W(Q,VoiceNo,C352_WAVE_START,Q_ReadWord(Q,V->WaveLinkPos+0x00)); // next bank
            Q_C352_W(Q,VoiceNo,C352_WAVE_LOOP, Q_ReadWord(Q,V->WaveLinkPos+0x04)); // start position of next link
        }
        else
        {
            // no link
            Q_C352_W(Q,VoiceNo,C352_FLAGS,    NextFlag);
            Q_C352_W(Q,VoiceNo,C352_WAVE_LOOP,Q_ReadWord(Q,V->WaveLinkPos+0x08)); // Next loop address
        }
    }
}
