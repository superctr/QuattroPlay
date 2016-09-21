/*
    Quattro - track commands & helper functions
*/
#include <stdio.h>

#include "quattro.h"
#include "track.h"
#include "voice.h"
#include "helper.h"
#include "tables.h"
#define TRACKCOMMAND(__name) void __name(Q_State* Q,int TrackNo,Q_Track* T,uint32_t* TrackPos,uint8_t Command)
#define WRITECALLBACK(__name) void __name(Q_State* Q,int TrackNo,Q_Track* T,uint32_t* TrackPos,int ChannelNo,int RegNo,uint16_t data)

#define LOGCMD Q_DEBUG("Trk %02x Pos %06x, Cmd: %02x (%s)\n",TrackNo,*TrackPos,Command,__func__)

// the following could be moved to track.c
// parse byte operands
uint8_t arg_byte(Q_State *Q,uint32_t* TrackPos)
{
    uint8_t r = Q->McuData[*TrackPos];
    *TrackPos += 1;
    return r;
}
// parse word operands
uint16_t arg_word(Q_State *Q,uint32_t* TrackPos)
{
    uint16_t r = (Q->McuData[*TrackPos+1]<<8) | (Q->McuData[*TrackPos]<<0);
    *TrackPos += 2;
    return r;
}
// parse track position operands
uint32_t arg_pos(Q_State *Q,uint32_t* TrackPos)
{
    uint32_t r = ((Q->McuData[*TrackPos+2]<<16) | (Q->McuData[*TrackPos+1]<<8) | (Q->McuData[*TrackPos]<<0)) - Q->McuPosBase;
    *TrackPos += 3;
    return r;
}
// parse operands for conditional jumps / set register commands
uint16_t arg_operand(Q_State *Q,uint32_t* TrackPos,uint8_t mode)
{
    uint16_t val;
    if(mode&0x80)
    {
        // immediate operand
        if(mode&0x40)
            val = arg_word(Q,TrackPos);
        else
            val = arg_byte(Q,TrackPos);
    }
    else
    {
        // register operand
        val = Q->Register[arg_byte(Q,TrackPos)];
        if(mode&0x40) // indirect
            val = Q->Register[val];
    }
    return val;
}

// parse "write track" commands
// source: 0x51ac
void WriteTrack(Q_State* Q,int TrackNo,uint32_t* TrackPos)
{
    uint8_t dest = arg_byte(Q,TrackPos);
    uint8_t data = arg_byte(Q,TrackPos);
    if(dest&0x80) // indirect?
        data = Q->Register[data]&0xff;
    Q_WriteTrackInfo(Q,TrackNo,dest&0x1f,data);
}

// parse "write channel" commands
// source: 0x51f4
void WriteChannel(Q_State* Q,int TrackNo,Q_Track* T,uint32_t* TrackPos,uint8_t Command,uint8_t WordMode,Q_WriteCallback callback)
{
    int ChannelNo;
    uint8_t IndirectMode=0;
    uint8_t SingleMode=0;
    uint16_t mask = arg_byte(Q,TrackPos);
    uint8_t dest = arg_byte(Q,TrackPos);
    uint16_t data=0;
    if(dest&0x80) // indirect mode - read values from the registers pointed at by arguments.
        IndirectMode=1;
    dest &= 0x7f;
    if(Command&0x40) // read one argument only
    {
        if(IndirectMode || !WordMode)
            data = arg_byte(Q,TrackPos);
        else
            data = arg_word(Q,TrackPos);

        if(IndirectMode)
            data = Q->Register[data];
        SingleMode=1;
    }
    for(ChannelNo=0;ChannelNo<8;ChannelNo++)
    {
        if(mask&0x80)
        {
            if(!SingleMode)
            {
                if(IndirectMode || !WordMode)
                    data = arg_byte(Q,TrackPos);
                else
                    data = arg_word(Q,TrackPos);

                if(IndirectMode)
                    data = Q->Register[data];
            }

            // Clear upper byte of pitch and volume envelope
            if(dest == 0x06)
                T->Channel[ChannelNo].EnvNo = 0;
            if(dest == 0x07)
                T->Channel[ChannelNo].PitchEnvNo = 0;

            // some special cases to write voice pitch if dest == 0x4b or 0x4c
            if(dest == 0x4c && T->Channel[ChannelNo].Enabled)
            {
                T->Channel[ChannelNo].Voice->Pitch &= 0xff00;
                T->Channel[ChannelNo].Voice->Pitch |= data&0xff;
            }
            else if(dest == 0x4b && T->Channel[ChannelNo].Enabled)
            {
                T->Channel[ChannelNo].Voice->Pitch &= 0x00ff;
                T->Channel[ChannelNo].Voice->Pitch |= (data&0xff)<<8;
            }
            else
            {
                Q_WriteChannelInfo(Q,TrackNo,ChannelNo,(dest)&0x1f,data&0xff);
                if(WordMode && (dest == 0x06 || dest == 0x07))
                    Q_WriteChannelInfo(Q,TrackNo,ChannelNo,(dest+0x18)&0x1f,data>>8);
                else if(dest == 0)
                    T->Channel[ChannelNo].WaveNo = data;
                else if(WordMode)
                    Q_WriteChannelInfo(Q,TrackNo,ChannelNo,(dest+1)&0x1f,data>>8);
            }

            if(callback != NULL)
                callback(Q,TrackNo,T,TrackPos,ChannelNo,dest&0x1f,data);
        }
        mask<<=1;
    }
}

// write channel or macro preset
// source: 0x545c
void WriteMultiple(Q_State* Q,Q_Channel* ch,uint32_t* TrackPos)
{
    uint16_t mask = arg_word(Q,TrackPos);
    int regno = 2;

    if(mask & 0x8000)
        ch->WaveNo = arg_word(Q,TrackPos);

    mask<<=1;
    while(mask)
    {
        if(mask & 0x8000)
            *(uint8_t*)((void*)ch+Q_ChannelStructMap[regno]) = arg_byte(Q,TrackPos);

        regno++;
        mask<<=1;
    }
}

// add events to the channel allocated voice
// source: 0x59f8
void WriteKeyOnEvent(Q_State* Q,Q_Track *T,Q_Channel* ch,uint8_t EventMode,uint8_t data)
{
    if(!ch->Enabled)
        return;

    Q_Voice* V;
    Q_VoiceEvent* E;
    uint8_t EventPos;
    uint32_t MacroPos;
    int i;

    uint8_t MacroRes;

    // read macro if exists..
    ch->Source = ch;
    if(ch->MacroMap)
    {
        // the initial value is ignored (maybe because it's useless)
        MacroPos = Q->TableOffset[Q_TOFFSET_MACROTABLE] + (16 * (ch->MacroMap-1))+1;
        MacroRes=0;
        if(data == 0xff)
        {
            // last entry
            MacroRes = Q->McuData[MacroPos+0x0e];
        }
        else
        {
            for(i=0;i<7;i++)
            {
                if(data < Q->McuData[MacroPos+1])
                {
                    MacroRes = Q->McuData[MacroPos];
                    break;
                }
                MacroPos+=2;
            }
            if(!MacroRes)
                MacroRes = Q->McuData[MacroPos];
        }
        if(MacroRes)
            ch->Source = &Q->ChannelMacro[MacroRes];
        else
            ch->Source = ch;

        // ptblank song 0xaa
        EventMode = Q_EVENTMODE_NOTE;
    }

    // do the rest here...
    if(EventMode == Q_EVENTMODE_OFFSET)
        ch->NoteDelay = data;
    if(ch->Legato)
    {
        ch->Legato--;
        EventMode |= 0x80;
    }

    V = ch->Voice;

    if(V->EventCh == NULL || V->EventCh == ch)
        V->LastEvent++;
    else
        V->LastEvent = V->CurrEvent+1;

    V->EventCh = ch;
    EventPos = (V->LastEvent-1)&0x07;

    E = &V->Event[EventPos];
    E->Mode = EventMode;
    E->Time = T->UpdateTime + (ch->NoteDelay*0x40);
    E->Value = data;
    E->Volume = ch->VolumeReg ? &Q->Register[ch->VolumeReg] : &T->GlobalVolume;
    E->Channel = ch->Source;
}

// parse key-on commands
// source: 0x58f4
void WriteKeyOn(Q_State* Q,int TrackNo,Q_Track* T,uint32_t* TrackPos,uint8_t Command,uint8_t EventMode,uint8_t IndirectMode)
{
    uint8_t mask_byte = arg_byte(Q,TrackPos);
    uint8_t mask = mask_byte;
    uint8_t data=0;
    int SingleMode=0, ChannelNo;

    if(Command&0x40) // read one argument only
    {
        data = arg_byte(Q,TrackPos);
        if(IndirectMode)
            data = Q->Register[data];
        SingleMode=1;
    }

    ChannelNo = 0;
    Q_Channel *ch, *srcch;

    // flagged channels...
    for(ChannelNo=0;ChannelNo<8;ChannelNo++)
    {
        ch = &T->Channel[ChannelNo];
        ch->KeyOnFlag = mask&0x80;

        if(ch->KeyOnFlag)
        {
            if(!SingleMode)
            {
                data = arg_byte(Q,TrackPos);
                if(IndirectMode)
                    data = Q->Register[data];
            }

            if(!T->SkipTrack)
            {
                ch->KeyOnNote = data;
                ch->KeyOnType = EventMode|0x80;
                WriteKeyOnEvent(Q,T,ch,EventMode,data);
            }
        }
        mask<<=1;
    }

    // now handle linked channels...
    ChannelNo=0;
    uint8_t link_mask;
    mask = ~mask_byte;
    while(mask)
    {
        ch = &T->Channel[ChannelNo];
        if(mask&0x80 && ch->ChannelLink)
        {
            srcch = &T->Channel[(ch->ChannelLink-1)&0x07];
            link_mask = 0x80 >> (ch->ChannelLink-1);
            if((mask_byte & link_mask) && srcch->KeyOnFlag)
                WriteKeyOnEvent(Q,T,ch,EventMode,srcch->KeyOnNote);
        }

        ChannelNo++;
        mask<<=1;
    }
}

// ============================================================================
// Command 0x00
TRACKCOMMAND(tc_Nop)
{
    LOGCMD;
    // do nothing...
    return;
}
// ============================================================================
// Command 0x01
TRACKCOMMAND(tc_Write8)
{
    uint16_t dest = arg_word(Q,TrackPos);
    uint8_t data = arg_byte(Q,TrackPos);

    // Alpine Racer discards anything written here, probably other drivers too
    if(Q->McuVer < Q_MCUVER_Q00 && Q->McuType == Q_MCUTYPE_SS22)
        return;

    // Ace Driver tries to write a track count of 255. Obviously we don't have that many tracks.
    if(dest == 0x5484 || dest == 0x4484)
        Q->TrackCount = data > Q_MAX_TRACKS ? Q_MAX_TRACKS : data;
    else if(dest == 0x5482 || dest == 0x4482)
        Q->VoiceCount = data > Q_MAX_VOICES ? Q_MAX_VOICES : data;
    else
        Q_DEBUG("Byte write %02x to addr %04x\n",data,dest);
    return;
}
// Command 0x02
TRACKCOMMAND(tc_Write16)
{
    uint16_t dest = arg_word(Q,TrackPos);
    uint16_t data = arg_word(Q,TrackPos);
    if(dest == 0x2400)
        C352_write(&Q->Chip,0x200,data);
    else if(dest == 0x00dc)
        Q->BasePitch = data;

    Q_DEBUG("Word write %04x to addr %04x\n",data,dest);
    return;
}
// ============================================================================
// Command 0x03
TRACKCOMMAND(tc_WriteTrack)
{
    WriteTrack(Q,TrackNo,TrackPos);
}
// Command 0x07
TRACKCOMMAND(tc_WriteTrackVolume)
{
    WriteTrack(Q,TrackNo,TrackPos);
    T->VolumeSource=&T->TrackVolume;
}
// Command 0x06
TRACKCOMMAND(tc_WriteTrackTempo)
{
    WriteTrack(Q,TrackNo,TrackPos);
    T->TempoReg=0;
}
// Command 0x2d
TRACKCOMMAND(tc_WriteTrackTempo2)
{
    WriteTrack(Q,TrackNo,TrackPos);
    T->TempoMulFactor=0;
    T->TempoMode=0;
}
// ============================================================================
// Command 0x04
TRACKCOMMAND(tc_WriteChannel)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,NULL);
}
// Command 0x30
TRACKCOMMAND(tc_WriteChannelEnvWord)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,1,NULL);
}
// ============================================================================
// Command 0x0f
WRITECALLBACK(cb_WriteChannelMacro)
{
    if(data)
        T->Channel[ChannelNo].Source = &Q->ChannelMacro[data];
    else
        T->Channel[ChannelNo].Source = &T->Channel[ChannelNo];

    T->Channel[ChannelNo].PanMode=Q_PANMODE_IMM;
}
TRACKCOMMAND(tc_WriteChannelMacro)
{
    LOGCMD;
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelMacro);
}
// ============================================================================
// Command 0x08
WRITECALLBACK(cb_WriteChannelPan)
{
    T->Channel[ChannelNo].PanMode=Q_PANMODE_IMM;
}
TRACKCOMMAND(tc_WriteChannelPan)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelPan);
}
// Command 0x1a
WRITECALLBACK(cb_WriteChannelPanReg)
{
    T->Channel[ChannelNo].PanMode=Q_PANMODE_REG;
}
TRACKCOMMAND(tc_WriteChannelPanReg)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelPanReg);
}
// Command 0x29
WRITECALLBACK(cb_WriteChannelPanEnv)
{
    T->Channel[ChannelNo].PanMode=Q_PANMODE_ENV_SET;
}
TRACKCOMMAND(tc_WriteChannelPanEnv)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelPanEnv);
}
// Command 0x2a
WRITECALLBACK(cb_WriteChannelPosEnv)
{
    T->Channel[ChannelNo].PanMode=Q_PANMODE_POSENV_SET;
}
TRACKCOMMAND(tc_WriteChannelPosEnv)
{
    LOGCMD;
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelPosEnv);
}
// Command 0x2b
WRITECALLBACK(cb_WriteChannelPosReg)
{
    T->Channel[ChannelNo].PanMode=Q_PANMODE_POSREG;
}
TRACKCOMMAND(tc_WriteChannelPosReg)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelPosReg);
}
// ============================================================================
WRITECALLBACK(cb_WriteChannelWave)
{
    T->Channel[ChannelNo].WaveNo |= 0x8000;
}
// Command 0x1b
TRACKCOMMAND(tc_WriteChannelWaveWord)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,1,cb_WriteChannelWave);
}
// Command 0x1c, 0x28
TRACKCOMMAND(tc_WriteChannelWaveByte)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_WriteChannelWave);
}
// ============================================================================
WRITECALLBACK(cb_InitChannel)
{
    Q_ChannelClear(Q,TrackNo,ChannelNo);
    uint8_t VoiceNo = T->Channel[ChannelNo].VoiceNo&0x1f;

    T->Channel[ChannelNo].Voice = &Q->Voice[VoiceNo];

    Q_VoiceSetPriority(Q,VoiceNo,TrackNo,ChannelNo,data);

    // check for other tracks
    uint16_t VPriority = Q_VoiceGetPriority(Q,VoiceNo,NULL,NULL);

    // no higher priority tracks on the voice? if so, allocate
    if(VPriority == data)
        Q_VoiceSetChannel(Q,VoiceNo,TrackNo,ChannelNo);

    T->Channel[ChannelNo].KeyOnType = 0; // for display
}
// Command 0x1d
TRACKCOMMAND(tc_InitChannel)
{
    WriteChannel(Q,TrackNo,T,TrackPos,Command,0,cb_InitChannel);
}
// ============================================================================
// Command 0x09 - read tempo from register (this will bypass the regular tempo calculation)
TRACKCOMMAND(tc_WriteTempoReg)
{
    T->TempoReg = arg_byte(Q,TrackPos);
    T->TempoSource = &Q->Register[T->TempoReg];
}
// Command 0x0a - read track volume from register
TRACKCOMMAND(tc_WriteVolumeReg)
{
    T->VolumeSource = &Q->Register[arg_byte(Q,TrackPos)];
    //printf("volumesource = %04x, %04x\n",*T->VolumeSource, T->VolumeSource-Q->Register);
}
// Command 0x0b - set accelerando/ritardando
TRACKCOMMAND(tc_WriteTempoMult)
{
    T->Tempo = arg_byte(Q,TrackPos);
    T->TempoFraction = 0;

    uint16_t tf = arg_byte(Q,TrackPos) * 328; // ceil(8000/327)
    T->TempoMulFactor = tf>>8;
    T->TempoMode = 0xff;
}
// Command 0x0c - write and enable a tempo sequence
TRACKCOMMAND(tc_WriteTempoSeq)
{
    uint8_t count = arg_byte(Q,TrackPos);

    T->TempoMode = count;
    T->TempoSeqStart = *TrackPos;
    T->TempoSeqPos = *TrackPos;

    *TrackPos += count;
}
// ============================================================================
// Command 0x0d - start subtrack with ID read from track
TRACKCOMMAND(tc_StartTrack)
{
    uint8_t slot = arg_byte(Q,TrackPos);
    uint16_t id = arg_word(Q,TrackPos);

    // prevent race condition
    // following code if uncommented will fix the issue detailed in track.c
    // line 77, but is untested...
#if 0
    if(Q->SongRequest[slot]&Q_TRACK_STATUS_BUSY)
        Q_TrackDisable(Q,slot);
#endif

    Q->SongRequest[slot] = id | (Q_TRACK_STATUS_START | Q_TRACK_STATUS_SUB);
    Q->ParentSong[slot] = TrackNo;
}
// Command 0x0e - starts subtrack with ID read from a register
TRACKCOMMAND(tc_StartTrackReg)
{
    uint8_t slot = arg_byte(Q,TrackPos);
    uint16_t id = Q->Register[arg_byte(Q,TrackPos)];

    Q->SongRequest[slot] = id | (Q_TRACK_STATUS_START | Q_TRACK_STATUS_SUB);
    Q->ParentSong[slot] = TrackNo;
}
// ============================================================================
// Command 0x10 - jump to address
TRACKCOMMAND(tc_Jump)
{
    *TrackPos = arg_pos(Q,TrackPos);

    Q_LoopDetectionJumpCheck(Q,TrackNo);
}
// Command 0x11 - jump to subroutine
TRACKCOMMAND(tc_JumpSub)
{
    uint32_t jump = arg_pos(Q,TrackPos);
    T->SubStack[T->SubStackPos] = *TrackPos;
    T->SubStackPos++;
    *TrackPos = jump;
}
// Command 0x12 - repeat section
TRACKCOMMAND(tc_Repeat)
{
    uint8_t count = arg_byte(Q,TrackPos);
    uint32_t jump = arg_pos(Q,TrackPos);
    int8_t pos = T->RepeatStackPos;

    if(pos > 0 && T->RepeatStack[pos-1] == *TrackPos)
    {
        // loop address stored in stack
        pos--;
        if(--T->RepeatCount[pos] > 0)
            *TrackPos = jump;
        else
            T->RepeatStackPos=pos;
    }
    else
    {
        // new loop
        T->RepeatStack[pos] = *TrackPos;
        T->RepeatCount[pos] = count;
        T->RepeatStackPos++;

        *TrackPos = jump;
    }
}
// Command 0x13 - decrease counter then jump if 0.
TRACKCOMMAND(tc_Loop)
{
    uint8_t count = arg_byte(Q,TrackPos);
    uint32_t jump = arg_pos(Q,TrackPos);
    int8_t pos = T->LoopStackPos;

    if(pos > 0 && T->LoopStack[pos-1] == *TrackPos)
    {
        // loop address stored in stack
        pos--;
        if(--T->LoopCount[pos] == 0)
        {
            T->LoopStackPos=pos;
            *TrackPos = jump;
        }
    }
    else
    {
        // new loop
        T->LoopStack[pos] = *TrackPos;
        T->LoopCount[pos] = count;
        T->LoopStackPos++;
    }
}
// Command 0x14 - returns from subroutine OR stops a track
TRACKCOMMAND(tc_Return)
{
    int8_t pos = T->SubStackPos;
    if(pos > 0)
    {
        pos--;
        *TrackPos = T->SubStack[pos];
        T->SubStackPos = pos;
        return;
    }

    Q_LoopDetectionCheck(Q,TrackNo,1);
    Q_TrackDisable(Q,TrackNo);
}
// Command 0x15 - stop track
TRACKCOMMAND(tc_Stop)
{
    Q_LoopDetectionCheck(Q,TrackNo,1);
    Q_TrackDisable(Q,TrackNo);
}
// ============================================================================
// Command 0x17 - write song message
TRACKCOMMAND(tc_WriteMessage)
{
    uint8_t *pos = (uint8_t*) Q->SongMessage, data;
    do
    {
        data = arg_byte(Q,TrackPos);
        *pos++ = data;
    }
    while(data != 0);
}
// ============================================================================
// Command 0x18 - write a channel preset
TRACKCOMMAND(tc_WriteChannelMultiple)
{
    Q_Channel *ch = &T->Channel[arg_byte(Q,TrackPos)];
    WriteMultiple(Q,ch,TrackPos);
}
// Command 0x19 - write a macro preset
TRACKCOMMAND(tc_WriteMacroMultiple)
{
    Q_Channel *ch = &Q->ChannelMacro[arg_byte(Q,TrackPos)];
    WriteMultiple(Q,ch,TrackPos);
}
// ============================================================================
// Command 0x1e - set/perform arithmetic on register
TRACKCOMMAND(tc_SetReg)
{
    uint16_t dest, source;
    uint8_t mode = arg_byte(Q,TrackPos);
    uint32_t reg;

    // destination register no
    dest = arg_byte(Q,TrackPos);
    if(mode&0x40) // indirect
        dest = Q->Register[dest];

    source = arg_operand(Q,TrackPos,mode<<2);

    reg = Q->Register[dest];
    Q->SetRegFlags = 0;

    switch(mode&0x0f)
    {
    default:
    case 0: // store
        reg = source;break;
    case 1: // add
        reg += source;break;
    case 2: // subtract
        reg -= source;break;
    case 3: // multiply
        reg *= source;
        if(reg > 0xffff)
            reg >>= 16;
        break;
    case 4: // divide
        if(source)
            reg/=source;
        break;
    case 6: // randomize
        reg = Q_GetRandom(&Q->LFSR1); // followed by modulo
    case 5: // modulo
        if(source)
            reg%=source;
        break;
    case 7: // and
        reg &= source;break;
    case 8: // or
        reg |= source;break;
    case 9: // xor
        reg ^= source;
        reg &= 0xffff;break;
    }
    // carry flag
    if(reg>0xffff)
        Q->SetRegFlags |= 1;
    // negate flag
    if(reg&0x8000)
        Q->SetRegFlags |= 2;

    Q->Register[dest] = reg&0xffff;
}
// ============================================================================
// Command 0x1f - Conditional Jump
TRACKCOMMAND(tc_CJump)
{
    uint16_t op1,op2;
    uint8_t mode = arg_byte(Q,TrackPos);
    uint32_t jump1, jump2;
    int res;

    op1 = arg_operand(Q,TrackPos,mode);
    op2 = arg_operand(Q,TrackPos,mode<<2);

    jump1 = arg_pos(Q,TrackPos);
    jump2 = arg_pos(Q,TrackPos);

    switch(mode&0x0f)
    {
    default:
    case 0: // equal
        res = op1 == op2;break;
    case 1: // not equal
        res = op1 != op2;break;
    case 2: // greater or equal
        res = op1 >= op2;break;
    case 3: // lessser or equal
        res = op1 <= op2;break;
    case 4: // greater than
        res = op1 > op2;break;
    case 5: // lesser than
        res = op1 < op2;break;
    case 6: // carry clear
        res = ~Q->SetRegFlags&1;break;
    case 7: // carry set
        res = Q->SetRegFlags&1;break;
    case 8: // negate clear
        res = ~Q->SetRegFlags&2;break;
    case 9: // negate set
        res = Q->SetRegFlags&2;break;
    }

    if(res)
        *TrackPos = jump1;
    else
        *TrackPos = jump2;
}
// ============================================================================
// Command 0x2c, 0x2e

// 0x2c is supposed to changed to change the handler for driver call 0x2e
//      (pan position update), but it is bugged - seemingly in all H8 drivers.
// 0x2e reads 0x40 bytes from the tracks and stores them in a song param area,
//      and then changes the handler for driver call 0x2c (pan update)
TRACKCOMMAND(tc_Dummy)
{
    LOGCMD;

#ifdef DEBUG
    uint8_t val = arg_byte(Q,TrackPos);
    Q_DEBUG("Dummy function %02x, arg %02x\n",Command,val);
#endif

}
TRACKCOMMAND(tc_Invalid)
{
    LOGCMD;
    Q_DEBUG("Track %02x Invalid command %02x at position %06x, stopping track\n",TrackNo,Command,*TrackPos);
    Q_TrackDisable(Q,TrackNo);
}
// ============================================================================
// Command 0x2f
TRACKCOMMAND(tc_Memory)
{
    LOGCMD;

#ifdef DEBUG
    uint8_t mode = arg_byte(Q,TrackPos);
    uint8_t reg = arg_byte(Q,TrackPos);
    uint32_t pos = arg_pos(Q,TrackPos)+Q->McuPosBase;
    Q_DEBUG("Register %02x, %s %s memory address %06x\n",reg,mode&1 ? "word" : "byte",mode&2 ? "read from" : "write to",pos);
#endif
}
// ============================================================================
// Command 0x20-0x27
TRACKCOMMAND(tc_KeyOn)
{
    uint8_t EventMode = (Command&6)>>1;
    uint8_t IndirectMode = Command&1;

    WriteKeyOn(Q,TrackNo,T,TrackPos,Command,EventMode,IndirectMode);

    if(T->KeyOnBuffer)
        T->KeyOnBuffer--;
    else if(!T->SkipTrack)
        T->TicksLeft=0; // set ticks left to 0 to end track update...
}
// ============================================================================
// source: 0x78a4 (sws2000)
Q_TrackCommand Q_TrackCommandTable[0x40] =
{
/* 00 */ tc_Nop,
/* 01 */ tc_Write8,
/* 02 */ tc_Write16,
/* 03 */ tc_WriteTrack,
/* 04 */ tc_WriteChannel,
/* 05 */ tc_Nop,
/* 06 */ tc_WriteTrackTempo,
/* 07 */ tc_WriteTrackVolume,
/* 08 */ tc_WriteChannelPan,
/* 09 */ tc_WriteTempoReg,
/* 0a */ tc_WriteVolumeReg,
/* 0b */ tc_WriteTempoMult,
/* 0c */ tc_WriteTempoSeq,
/* 0d */ tc_StartTrack,
/* 0e */ tc_StartTrackReg,
/* 0f */ tc_WriteChannelMacro,
/* 10 */ tc_Jump,
/* 11 */ tc_JumpSub,
/* 12 */ tc_Repeat,
/* 13 */ tc_Loop,
/* 14 */ tc_Return,
/* 15 */ tc_Stop,
/* 16 */ tc_Nop,
/* 17 */ tc_WriteMessage,
/* 18 */ tc_WriteChannelMultiple,
/* 19 */ tc_WriteMacroMultiple,
/* 1a */ tc_WriteChannelPanReg,
/* 1b */ tc_WriteChannelWaveWord,
/* 1c */ tc_WriteChannelWaveByte,
/* 1d */ tc_InitChannel,
/* 1e */ tc_SetReg,
/* 1f */ tc_CJump,
/* 20 */ tc_KeyOn, // note
/* 21 */ tc_KeyOn, // note (from register)
/* 22 */ tc_KeyOn, // volume
/* 23 */ tc_KeyOn, // volume (from register)
/* 24 */ tc_KeyOn, // delay
/* 25 */ tc_KeyOn, // delay (from register)
/* 26 */ tc_KeyOn, // pan
/* 27 */ tc_KeyOn, // pan (from register)
/* 28 */ tc_WriteChannelWaveByte, // used to write sample offset
/* 29 */ tc_WriteChannelPanEnv,
/* 2a */ tc_WriteChannelPosEnv,
/* 2b */ tc_WriteChannelPosReg,
/* 2c */ tc_Dummy,
/* 2d */ tc_WriteTrackTempo2,
/* 2e */ tc_Dummy,
/* 2f */ tc_Memory,
// added in driver ver Q02Nxxxx (don't know exact cutoff)
/* 30 */ tc_WriteChannelEnvWord,
// following are not known to be implemented in any version
/* 31 */ tc_Invalid,
/* 32 */ tc_Invalid,
/* 33 */ tc_Invalid,
/* 34 */ tc_Invalid,
/* 35 */ tc_Invalid,
/* 36 */ tc_Invalid,
/* 37 */ tc_Invalid,
/* 38 */ tc_Invalid,
/* 39 */ tc_Invalid,
/* 3a */ tc_Invalid,
/* 3b */ tc_Invalid,
/* 3c */ tc_Invalid,
/* 3d */ tc_Invalid,
/* 3e */ tc_Invalid,
/* 3f */ tc_Invalid,
};
