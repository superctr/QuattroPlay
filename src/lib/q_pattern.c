/*
    Track command parser for UI pattern display.
*/
#include <string.h>

#include "../qp.h"

#include "../drv/quattro.h"
#include "../drv/helper.h"
#include "../s2x/s2x.h"
#include "../s2x/track.h"
#include "q_pattern.h"


static Q_State *Q;
static S2X_State *S;

static struct QP_Pattern *P;

static uint8_t q_pattern_arg_byte(uint32_t* TrackPos)
{
    uint8_t r = Q->McuData[*TrackPos];
    *TrackPos += 1;
    return r;
}
// parse word operands
static uint16_t q_pattern_arg_word(uint32_t* TrackPos)
{
    uint16_t r = (Q->McuData[*TrackPos+1]<<8) | (Q->McuData[*TrackPos]<<0);
    *TrackPos += 2;
    return r;
}
// parse track position operands
static uint32_t q_pattern_arg_pos(uint32_t* TrackPos)
{
    uint32_t r = ((Q->McuData[*TrackPos+2]<<16) | (Q->McuData[*TrackPos+1]<<8) | (Q->McuData[*TrackPos]<<0)) - Q->McuPosBase;
    *TrackPos += 3;
    return r;
}
// parse operands for conditional jumps / set register commands
static uint16_t q_pattern_arg_operand(uint32_t* TrackPos,uint8_t mode,uint16_t *regs)
{
    uint16_t val;
    if(mode&0x80)
    {
        // immediate operand
        if(mode&0x40)
            val = q_pattern_arg_word(TrackPos);
        else
            val = q_pattern_arg_byte(TrackPos);
    }
    else
    {
        // register operand
        val = regs[q_pattern_arg_byte(TrackPos)];
        if(mode&0x40) // indirect
            val = regs[val];
    }
    return val;
}

static void q_generate(int TrackNo)
{
    static uint16_t regs[256];
    int i, left, skip;
    uint32_t pos, jump;
    uint8_t cmd, setflags;
    uint16_t mask, data, temp, dest, source, lfsr;
    int subpos, reppos, looppos;
    static uint32_t substack[Q_MAX_SUB_STACK], repstack[Q_MAX_REPEAT_STACK], loopstack[Q_MAX_LOOP_STACK];
    static uint8_t repcount[Q_MAX_REPEAT_STACK], loopcount[Q_MAX_LOOP_STACK];
    static uint8_t transpose[Q_MAX_TRKCHN];
    int maxcommands = 50000;

    P->len = 0;
    Q_Track* T = &Q->Track[TrackNo];
    if(~T->Flags & Q_TRACK_STATUS_BUSY)
        return;

    // copy paramters
    SDL_LockAudioDevice(Audio->dev);
    memcpy(regs,Q->Register,sizeof(Q->Register));
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

    setflags = Q->SetRegFlags;
    lfsr = Q->LFSR1;
    left = T->RestCount;
    pos = T->Position;
    SDL_UnlockAudioDevice(Audio->dev);

    // insert empty rows
    while(left--)
    {
        for(i=0;i<Q_MAX_TRKCHN;i++)
            P->pat[P->len][i] = -1;

        P->len++;

        if(P->len == 32)
            return;
    }

    while(P->len < 32)
    {
        if(pos>0x7ffff)
        {
            Q_DEBUG("WARNING: ui_pattern_disp read pos (%06x) (T=%02x at %06x)\n",pos,TrackNo,T->Position);
            return;
        }

        cmd = q_pattern_arg_byte(&pos);
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
                cmd = q_pattern_arg_byte(&pos);
                skip = cmd;
                break;
            case 0x17: // song message
                while(cmd!=0)
                    cmd = q_pattern_arg_byte(&pos);
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
                mask = q_pattern_arg_byte(&pos);
                dest = q_pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = q_pattern_arg_byte(&pos);
                i = 0;
                while(mask&0xff)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = q_pattern_arg_byte(&pos);

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
                mask = q_pattern_arg_byte(&pos);
                dest = q_pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = (dest&0x80) ? q_pattern_arg_byte(&pos) : q_pattern_arg_word(&pos);
                i = 0;
                while(mask&0xff)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = (dest&0x80) ? q_pattern_arg_byte(&pos) : q_pattern_arg_word(&pos);

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
                mask = q_pattern_arg_word(&pos);
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
                pos = q_pattern_arg_pos(&pos);
                break;
            case 0x11: // sub
                substack[subpos] = pos+3;
                pos = q_pattern_arg_pos(&pos);
                subpos++;
                break;
            case 0x12: // repeat
                dest = q_pattern_arg_byte(&pos);
                jump = q_pattern_arg_pos(&pos);
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
                dest = q_pattern_arg_byte(&pos);
                jump = q_pattern_arg_pos(&pos);
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
                data = q_pattern_arg_byte(&pos);
                uint32_t reg;
                // destination register no
                dest = q_pattern_arg_byte(&pos);
                if(data&0x40) // indirect
                    dest = regs[dest];
                source = q_pattern_arg_operand(&pos,data<<2,regs);
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
                data = q_pattern_arg_byte(&pos);
                uint16_t op1,op2;
                uint32_t jump1, jump2;
                int res;
                op1 = q_pattern_arg_operand(&pos,data,regs);
                op2 = q_pattern_arg_operand(&pos,data<<2,regs);
                jump1 = q_pattern_arg_pos(&pos);
                jump2 = q_pattern_arg_pos(&pos);
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
                mask = q_pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = q_pattern_arg_byte(&pos);
                for(i=0;i<Q_MAX_TRKCHN;i++)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = q_pattern_arg_byte(&pos);

                        // add transpose offset
                        if(((cmd&0x3f) == 0x20) && data < 0x7f)
                            data += transpose[i];

                        // write note
                        P->pat[P->len][i] = (data&0xff) | (dest<<8);
                    }
                    else
                        P->pat[P->len][i] = -1;
                    mask<<=1;
                }
                P->len++;
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
                    P->pat[P->len][i] = -1;
                P->len++;
                cmd--;
                if(P->len == 32)
                    return;
            }
        }
    }
}

// ============================================================================

static uint32_t posbase;

static uint8_t s2x_pattern_arg_byte(uint32_t* TrackPos)
{
    uint8_t r = S->Data[*TrackPos];
    *TrackPos += 1;
    return r;
}
// parse track position operands
static uint32_t s2x_pattern_arg_pos(uint32_t* TrackPos)
{
    uint32_t r = posbase;
    if(S->DriverType == S2X_TYPE_NA)
        r += (S->Data[*TrackPos]&0xff)|(S->Data[*TrackPos+1]<<8);
    else
        r += (S->Data[*TrackPos]<<8)|(S->Data[*TrackPos+1]&0xff);
    *TrackPos += 2;
    return r;
}
static uint32_t s2x_s86jump(uint32_t* TrackPos)
{
    uint32_t songtab = posbase + S->FMSongTab + (2*s2x_pattern_arg_byte(TrackPos));
    return posbase + ((S->Data[songtab]<<8)|(S->Data[songtab+1]&0xff));
}
static int16_t s2x_fmkeycode(uint8_t d)
{
    if((d&3) == 3)
        return 0x100;
    return ((d>>2)*3)+(d&3);
}

static void s2x_generate(int TrackNo)
{
    struct S2X_TrackCommandEntry* CmdTab = S2X_TrackCommandTable[S->DriverType];

    static uint16_t cjump;
    int i, left, skip;
    uint32_t pos, jump;
    uint8_t cmd;
    uint16_t mask, data, temp, dest;
    int subpos, reppos, looppos;
    static uint32_t substack[S2X_MAX_SUB_STACK], repstack[S2X_MAX_REPEAT_STACK], loopstack[S2X_MAX_LOOP_STACK];
    static uint8_t repcount[S2X_MAX_REPEAT_STACK], loopcount[S2X_MAX_LOOP_STACK];
    static uint8_t transpose[S2X_MAX_TRKCHN];
    int maxcommands = 50000;

    P->len = 0;
    S2X_Track* T = &S->Track[TrackNo];
    if(~T->Flags & S2X_TRACK_STATUS_BUSY)
        return;

    // copy paramters
    SDL_LockAudioDevice(Audio->dev);
    cjump = (S->CJump) ? 0x400 : T->Flags&0x400;
    memcpy(substack,T->SubStack,sizeof(T->SubStack));
    memcpy(repstack,T->RepeatStack,sizeof(T->RepeatStack));
    memcpy(loopstack,T->LoopStack,sizeof(T->LoopStack));
    memcpy(repcount,T->RepeatCount,sizeof(T->RepeatCount));
    memcpy(loopcount,T->LoopCount,sizeof(T->LoopCount));
    subpos = T->SubStackPos;
    reppos = T->RepeatStackPos;
    looppos = T->LoopStackPos;
    for(i=0;i<S2X_MAX_TRKCHN;i++)
        transpose[i] = T->Channel[i].Vars[S2X_CHN_TRS];

    left = T->RestCount;
    posbase = T->PositionBase;
    pos = T->Position+posbase;
    SDL_UnlockAudioDevice(Audio->dev);

    // insert empty rows
    while(left--)
    {
        for(i=0;i<S2X_MAX_TRKCHN;i++)
            P->pat[P->len][i] = -1;

        P->len++;

        if(P->len == 32)
            return;
    }

    while(P->len < 32)
    {
        if(pos>0x3fffff)
        {
            Q_DEBUG("WARNING: ui_pattern_disp read pos (%06x) (T=%02x at %06x)\n",pos,TrackNo,T->Position);
            return;
        }

        cmd = s2x_pattern_arg_byte(&pos);
        maxcommands--;

        if(cmd<0x80 && maxcommands)
        {
            if((cmd&0x3f)<0x25)
                skip=CmdTab[cmd&0x3f].type;
            else
                skip=S2X_CMD_END;
            switch(skip)
            {
            case S2X_CMD_END:
                return;
            case S2X_CMD_CHN:
            case S2X_CMD_WAV:
            case S2X_CMD_FRQ:
            case S2X_CMD_TRS:
                mask = s2x_pattern_arg_byte(&pos);
                if(cmd&0x40)
                    temp = s2x_pattern_arg_byte(&pos);
                for(i=0;i<S2X_MAX_TRKCHN;i++)
                {
                    if(mask&0x80)
                    {
                        if(cmd&0x40)
                            data = temp;
                        else
                            data = s2x_pattern_arg_byte(&pos);
                        // transpose write
                        if(skip == S2X_CMD_TRS)
                            transpose[i] = data;
                        else if(skip == S2X_CMD_FRQ && S->DriverType == S2X_TYPE_SYSTEM86)
                            P->pat[P->len][i] = s2x_fmkeycode(data&0xff);
                        else if(skip == S2X_CMD_FRQ && data<0xff)
                            P->pat[P->len][i] = ((data+transpose[i])&0xff);
                        else if(skip == S2X_CMD_FRQ)
                            P->pat[P->len][i] = (data&0xff) | 0x100;
                        else if(skip == S2X_CMD_WAV)
                            P->pat[P->len][i] = (data&0xff) | 0x200;
                    }
                    else if(skip == S2X_CMD_FRQ || skip == S2X_CMD_WAV)
                        P->pat[P->len][i] = -1;
                    mask<<=1;
                }
                if(skip == S2X_CMD_FRQ || skip == S2X_CMD_WAV)
                    P->len++;
                break;
            case S2X_CMD_CJUMP:
                if(!cjump)
                {
                    pos+=2;
                    cjump=1;
                    break;
                }
            case S2X_CMD_JUMP: // jump
                pos = s2x_pattern_arg_pos(&pos);
                break;
            case S2X_CMD_CALL: // sub
                substack[subpos] = pos+2-posbase;
                pos = s2x_pattern_arg_pos(&pos);
                subpos++;
                break;
            case S2X_CMD_JUMP86: // jump
                pos = s2x_s86jump(&pos);
                break;
            case S2X_CMD_CALL86: // sub
                substack[subpos] = pos+1-posbase;
                pos = s2x_s86jump(&pos);
                subpos++;
                break;
            case S2X_CMD_REPT: // repeat
                dest = s2x_pattern_arg_byte(&pos);
                jump = s2x_pattern_arg_pos(&pos);
                data = reppos;
                if(data > 0 && repstack[data-1] == pos-posbase)
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
                    repstack[data] = pos-posbase;
                    repcount[data] = dest;
                    reppos++;
                    pos = jump;
                }
                break;
            case S2X_CMD_LOOP: // loop
                dest = s2x_pattern_arg_byte(&pos);
                jump = s2x_pattern_arg_pos(&pos);
                data = looppos;

                if(data > 0 && loopstack[data-1] == pos-posbase)
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
                    loopstack[data] = pos-posbase;
                    loopcount[data] = dest;
                    looppos++;
                }
                break;
            case S2X_CMD_RET:
                if(!subpos)
                    return;
                pos = substack[--subpos]+posbase;
                break;
            case S2X_CMD_EMPTY:
                for(i=0;i<S2X_MAX_TRKCHN;i++)
                    P->pat[P->len][i] = -1;
                P->len++;
                break;
            default:
                pos += skip-1;
                break;
            }
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
                for(i=0;i<S2X_MAX_TRKCHN;i++)
                    P->pat[P->len][i] = -1;
                P->len++;
                cmd--;
                if(P->len == 32)
                    return;
            }
        }
    }
}

// ============================================================================

void QP_PatternGenerate(int TrackNo,struct QP_Pattern* Pat)
{
    P = Pat;
    switch(DriverInterface->Type)
    {
    case DRIVER_QUATTRO:
        Q = DriverInterface->Driver;
        q_generate(TrackNo);
        break;
    case DRIVER_SYSTEM2:
        S = DriverInterface->Driver;
        s2x_generate(TrackNo);
        break;
    default:
        P->len=0;
        break;
    }
    Q = DriverInterface->Driver;
}
