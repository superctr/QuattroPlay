/*
    Track command parser for UI pattern display.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ui.h"
#include "quattro.h"

uint8_t pattern_arg_byte(uint32_t* TrackPos)
{
    uint8_t r = QDrv->McuData[*TrackPos];
    *TrackPos += 1;
    return r;
}
// parse word operands
uint16_t pattern_arg_word(uint32_t* TrackPos)
{
    uint16_t r = (QDrv->McuData[*TrackPos+1]<<8) | (QDrv->McuData[*TrackPos]<<0);
    *TrackPos += 2;
    return r;
}
// parse track position operands
uint32_t pattern_arg_pos(uint32_t* TrackPos)
{
    uint32_t r = ((QDrv->McuData[*TrackPos+2]<<16) | (QDrv->McuData[*TrackPos+1]<<8) | (QDrv->McuData[*TrackPos]<<0)) - QDrv->McuPosBase;
    *TrackPos += 3;
    return r;
}
// parse operands for conditional jumps / set register commands
uint16_t pattern_arg_operand(uint32_t* TrackPos,uint8_t mode,uint16_t *regs)
{
    uint16_t val;
    if(mode&0x80)
    {
        // immediate operand
        if(mode&0x40)
            val = pattern_arg_word(TrackPos);
        else
            val = pattern_arg_byte(TrackPos);
    }
    else
    {
        // register operand
        val = regs[pattern_arg_byte(TrackPos)];
        if(mode&0x40) // indirect
            val = regs[val];
    }
    return val;
}

void ui_pattern_disp(int TrackNo)
{
    uint16_t regs[256];
    int i, left, skip;
    uint32_t pos, jump;
    uint8_t cmd, setflags;
    uint16_t mask, data, temp, dest, source, lfsr;
    int subpos, reppos, looppos;
    uint32_t substack[Q_MAX_SUB_STACK], repstack[Q_MAX_REPEAT_STACK], loopstack[Q_MAX_LOOP_STACK];
    uint8_t repcount[Q_MAX_REPEAT_STACK], loopcount[Q_MAX_LOOP_STACK];
    uint8_t transpose[Q_MAX_TRKCHN];
    int maxcommands = 50000;

    trackpattern_length = 0;
    Q_Track* T = &QDrv->Track[TrackNo];
    if(~T->Flags & Q_TRACK_STATUS_BUSY)
        return;

    // copy paramters
    memcpy(regs,QDrv->Register,sizeof(QDrv->Register));
    memcpy(substack,T->SubStack,sizeof(T->SubStack));
    memcpy(repstack,T->RepeatStack,sizeof(T->RepeatStack));
    memcpy(loopstack,T->LoopStack,sizeof(T->LoopStack));
    memcpy(repcount,T->RepeatCount,sizeof(T->RepeatCount));
    memcpy(loopcount,T->LoopCount,sizeof(T->LoopCount));
    subpos = T->SubStackPos;
    reppos = T->RepeatStackPos;
    looppos = T->LoopStackPos;
    for(i=0;i<Q_MAX_TRKCHN;i++)
        transpose[i] = T->Channel[i].Transpose;

    setflags = QDrv->SetRegFlags;
    lfsr = QDrv->LFSR1;
    left = T->RestCount;
    pos = T->Position;

    // insert empty rows
    while(left--)
    {
        for(i=0;i<Q_MAX_TRKCHN;i++)
            trackpattern[trackpattern_length][i] = -1;

        trackpattern_length++;

        if(trackpattern_length == 32)
            return;
    }

    while(trackpattern_length < 32)
    {
        cmd = pattern_arg_byte(&pos);
        maxcommands--;

        if(cmd<0x80 && maxcommands)
        {
            skip=0;
            switch(cmd&0x3f)
            {
            default: // don't continue if we encounter an unknown command
            case 0x15:
                return;
                break;
            case 0x14:
                if(!subpos)
                    return;
                pos = substack[--subpos];
                break;
            // do nothing for these
            case 0x00:
            case 0x16:
                break;
            case 0x09:
            case 0x0a:
            case 0x2c:
                skip=1;break;
            case 0x03:
            case 0x06:
            case 0x07:
            case 0x0b:
            case 0x0e:
            case 0x2d:
            case 0x2e:
                skip=2;break;
            case 0x01:
            case 0x0d:
                skip=3;break;
            case 0x02:
                skip=4;break;
            case 0x2f:
                skip=5;break;
            case 0x0c: // tempo sequence
                cmd = pattern_arg_byte(&pos);
                skip = cmd;
                break;
            case 0x17: // song message
                while(cmd!=0)
                    cmd = pattern_arg_byte(&pos);
                break;
            // channel write (byte argument)
            case 0x04:
            case 0x08:
            case 0x0f:
            case 0x1a:
            case 0x1c:
            case 0x1d:
            case 0x28:
            case 0x29:
            case 0x2a:
            case 0x2b:
                mask = pattern_arg_byte(&pos);
                dest = pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = pattern_arg_byte(&pos);
                i = 0;
                while(mask&0xff)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = pattern_arg_byte(&pos);

                        if(dest&0x80)
                            data = regs[data];

                        // transpose write
                        if((dest&0x7f) == 0x0b)
                            transpose[i] = data;
                    }
                    mask<<=1;
                    i++;
                }
                break;
            // channel write (word argument)
            case 0x1b:
            case 0x30:
                mask = pattern_arg_byte(&pos);
                dest = pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = (dest&0x80) ? pattern_arg_byte(&pos) : pattern_arg_word(&pos);
                i = 0;
                while(mask&0xff)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = (dest&0x80) ? pattern_arg_byte(&pos) : pattern_arg_word(&pos);

                        if(dest&0x80)
                            data = regs[data&0xff];

                        // transpose write
                        if((dest&0x7f) == 0x0b)
                            transpose[i] = data;
                    }
                    mask<<=1;
                    i++;
                }
                break;
            // channel/macro write
            case 0x18:
            case 0x19:
                pos++;
                mask = pattern_arg_word(&pos);
                if(mask & 0x8000)
                    pos+=2;
                mask<<=1;
                while(mask)
                {
                    skip++;
                    mask<<=1;
                }
                break;
            case 0x10: // jump
                pos = pattern_arg_pos(&pos);
                break;
            case 0x11: // sub
                substack[subpos] = pos+3;
                pos = pattern_arg_pos(&pos);
                subpos++;
                break;
            case 0x12: // repeat
                dest = pattern_arg_byte(&pos);
                jump = pattern_arg_pos(&pos);
                data = reppos;
                if(data > 0 && repstack[data-1] == pos)
                {
                    // loop address stored in stack
                    data--;
                    if(--repcount[data] > 0)
                        pos = jump;
                    else
                        reppos=data;
                }
                else
                {
                    // new loop
                    repstack[data] = pos;
                    repcount[data] = dest;
                    reppos++;
                    pos = jump;
                }
                break;
            case 0x13: // loop
                dest = pattern_arg_byte(&pos);
                jump = pattern_arg_pos(&pos);
                data = looppos;

                if(data > 0 && loopstack[data-1] == pos)
                {
                    // loop address stored in stack
                    data--;
                    if(--loopcount[data] == 0)
                    {
                        looppos=data;
                        pos = jump;
                    }
                }
                else
                {
                    // new loop
                    loopstack[data] = pos;
                    loopcount[data] = dest;
                    looppos++;
                }
                break;
            case 0x1e: // set reg
                data = pattern_arg_byte(&pos);
                uint32_t reg;
                // destination register no
                dest = pattern_arg_byte(&pos);
                if(data&0x40) // indirect
                    dest = regs[dest];
                source = pattern_arg_operand(&pos,data<<2,regs);
                reg = regs[dest];
                setflags = 0;
                switch(data&0x0f)
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
                    reg = Q_GetRandom(&lfsr);
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
                    setflags |= 1;
                // negate flag
                if(reg&0x8000)
                    setflags |= 2;
                regs[dest] = reg&0xffff;
                break;
            case 0x1f: // conditional jump
                data = pattern_arg_byte(&pos);
                uint16_t op1,op2;
                uint32_t jump1, jump2;
                int res;
                op1 = pattern_arg_operand(&pos,data,regs);
                op2 = pattern_arg_operand(&pos,data<<2,regs);
                jump1 = pattern_arg_pos(&pos);
                jump2 = pattern_arg_pos(&pos);
                switch(data&0x0f)
                {
                default:
                case 0:
                    res = op1 == op2;break;
                case 1:
                    res = op1 != op2;break;
                case 2:
                    res = op1 >= op2;break;
                case 3:
                    res = op1 <= op2;break;
                case 4:
                    res = op1 > op2;break;
                case 5:
                    res = op1 < op2;break;
                case 6: // carry clear
                    res = ~setflags&1;break;
                case 7: // carry set
                    res = setflags&1;break;
                case 8: // negate clear
                    res = ~setflags&2;break;
                case 9: // negate set
                    res = setflags&2;break;
                }
                if(res)
                    pos = jump1;
                else
                    pos = jump2;
                break;
            // key on (has all the pattern data we want)
            case 0x20:
            case 0x21:
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
                dest = cmd&7;
                mask = pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = pattern_arg_byte(&pos);
                for(i=0;i<Q_MAX_TRKCHN;i++)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = pattern_arg_byte(&pos);

                        // add transpose offset
                        if(((cmd&0x3f) == 0x20) && data < 0x7f)
                            data += transpose[i];

                        // write note
                        trackpattern[trackpattern_length][i] = (data&0xff) | (dest<<8);
                    }
                    else
                        trackpattern[trackpattern_length][i] = -1;
                    mask<<=1;
                }
                trackpattern_length++;
                break;
            }
            pos += skip;
        }
        else
        {
            // break if we hit command limit
            if(maxcommands == 0)
                cmd = 0x7f;

            // empty row
            cmd &= 0x7f;
            cmd++;
            while(cmd)
            {
                // rest...
                for(i=0;i<Q_MAX_TRKCHN;i++)
                    trackpattern[trackpattern_length][i] = -1;
                trackpattern_length++;
                cmd--;
                if(trackpattern_length == 32)
                    return;
            }
        }
    }
}
