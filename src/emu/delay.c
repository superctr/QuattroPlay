/*
    Simple delay line / reverb DSP for QuattroPlay

    Register map
    00-1f : Delay lines
    20-23 : Delay preset setting

    Delay line:
    00 : Start address
    01 : End address (Delay time = end - start)
    02 : Delay feedback
    03 : Output LPF cutoff

    Preset setting (Writes here will reset the delay line configuration):
    20 : Preset setting (0=No preset)
    21 : Base feedback
    22 : Left delay base
    23 : Right delay base
*/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "delay.h"

int DelayDSP_init(DelayDSP *c)
{
    memset(c->delay_memory,0,sizeof(c->delay_memory));
    memset(c->line,0,sizeof(DelayDSP_Line)*DELAYDSP_LINES);
    c->in_count = 0;
    c->enable_count = 0;

    c->preset_time1 = 0x100;
    c->preset_time2 = 0x101;
    c->preset_set = 1;
    c->preset_feedback = 220<<8;

    DelayDSP_set_preset(c);

    return -1;
}

// Set input channels.
// a should point to a struct of int16_t values, one for each channel.
// in_count is the total amount of channels.
// enable_count is the amount of channels which will be processed by the delay. Only tested value is 2
void DelayDSP_set_input(DelayDSP *c,int16_t *a,int in_count,int enable_count)
{
    c->in_count = in_count;
    c->enable_count = enable_count;
    c->input = a;
}

int16_t DelayDSP_update_line(DelayDSP *c,int line,int16_t input)
{
    DelayDSP_Line *d = &c->line[line];

    int16_t output_sample = (c->delay_memory[d->pos] * d->feedback)>>16;

    // slowly reduce to 0
    if(output_sample > 0)
        output_sample--;
    if(output_sample < 0)
        output_sample++;

    c->delay_memory[d->pos] = output_sample + input;

    if(d->pos == d->line_end)
        d->pos = d->line_start;
    else
        d->pos++;

    d->filter_state = (int32_t)((output_sample - d->filter_state)*d->input_filter)>>16;
    return d->filter_state;
}

void DelayDSP_update(DelayDSP *c)
{
    int i;

    if(!c->in_count)
        return;

    // sample all inputs
    for(i=0;i<c->in_count;i++)
        c->output[i] = c->input[i];

    if(!c->enable_count)
        return;

    // apply delay for the non-bypass inputs
    for(i=0;i<DELAYDSP_LINES;i++)
    {
        c->output[i%c->enable_count] += DelayDSP_update_line(c,i,c->input[i%c->enable_count]>>1);
    }
}

static uint16_t DelayRegMap[4] = {
    offsetof(DelayDSP_Line,line_start),
    offsetof(DelayDSP_Line,line_end),
    offsetof(DelayDSP_Line,feedback),
    offsetof(DelayDSP_Line,input_filter),
};

static uint32_t DelayPresetRegMap[4] = {
    offsetof(DelayDSP,preset_set),
    offsetof(DelayDSP,preset_feedback),
    offsetof(DelayDSP,preset_time1),
    offsetof(DelayDSP,preset_time2),
};

void DelayDSP_write(DelayDSP *c, uint16_t addr, uint16_t data)
{
    if(addr < 0x20)
    {
        *(uint16_t*)((void*)&c->line[addr/4]+DelayRegMap[addr%4]) = data;
        c->line[addr/4].pos = c->line[addr/4].line_start;
    }
    else if(addr < 0x24)
    {
        *(uint16_t*)((void*)c+DelayPresetRegMap[addr%4]) = data;
        DelayDSP_set_preset(c);
    }
}

uint16_t DelayDSP_read(DelayDSP *c, uint16_t addr)
{
    if(addr < 0x20)
        return *(uint16_t*)((void*)&c->line[addr/4]+DelayRegMap[addr%4]);
    else if(addr < 0x24)
        return *(uint16_t*)((void*)c+DelayPresetRegMap[addr%4]);
    else
        return 0;
}

static struct {
    uint16_t time_base[4];
    uint16_t filter[4];
} PresetArray[] = {
    {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
    },
    {
        { 7, 11, 13, 17 },
        { 0x1500,0x1000,0xa00,0x800 },
    },

};

void DelayDSP_set_preset(DelayDSP *c)
{
    int i;
    int preset = c->preset_set;
    uint16_t addr = 0;

    for(i=0;i<DELAYDSP_LINES;i++)
    {
        int16_t temp = PresetArray[preset].time_base[i/2] * ((i&1) ? c->preset_time2 : c->preset_time1);
        c->line[i].line_start = addr;
        c->line[i].line_end = temp ? addr+temp-1 : addr;
        addr += temp;
        c->line[i].input_filter = temp ? PresetArray[preset].filter[i/2] : 0;
        c->line[i].feedback = temp ? c->preset_feedback : 0;
        c->line[i].pos = c->line[i].line_start;
    }

}
