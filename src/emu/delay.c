/*
    Simple delay line / reverb DSP for QuattroPlay

    8 delay lines (4 per channel)

    Register map
    00-1f : Delay lines
    20-27 : Delay preset setting

    Delay line:
    00 : Start address
    01 : End address (Delay time = end - start)
    02 hi : Delay feedback
    02 lo : Delay output volume
    03 : Output LPF cutoff

    Preset setting (Writes here will reset the delay line configuration):
    20 : Preset setting (0=No preset)
    21 : Delay base
    22 : L/R delay offset
    23 : Feedback volume
    24 : Output volume
    25 : Output filter cutoff
    26 : -
    27 : -
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

    // default settings for a nice room reverb
    c->preset_set = 1;
    c->preset_time1 = 170;
    c->preset_time2 = 1;
    c->preset_feedback = 250;
    c->preset_vol = 100;
    c->preset_filter = 100;

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

    uint8_t out_volume = d->feedback&0xff;
    uint8_t feedback = d->feedback>>8;

    int16_t output_sample = (c->delay_memory[d->pos] * out_volume)>>8;
    int16_t feedback_sample = (c->delay_memory[d->pos] * feedback)>>8;

    // slowly reduce to 0
    if(feedback_sample > 0)
        feedback_sample--;
    if(feedback_sample < 0)
        feedback_sample++;

    c->delay_memory[d->pos] =  feedback_sample + input;

    if(d->pos == d->line_end)
        d->pos = d->line_start;
    else
        d->pos++;

    d->filter_state += (int32_t)((output_sample - (d->filter_state>>16))*d->input_filter);
    return d->filter_state>>16;
}

void DelayDSP_update(DelayDSP *c)
{
    int i;

    if(!c->in_count)
        return;

    // sample all inputs
    for(i=0;i<c->in_count;i++)
        c->output[i] = c->input[i]>>2;

    if(!c->enable_count)
        return;

    // apply delay for the non-bypass inputs
    for(i=0;i<DELAYDSP_LINES;i++)
    {
        c->output[i%c->enable_count] += DelayDSP_update_line(c,i,c->input[i%c->enable_count]>>2);
    }
}

static uint16_t DelayRegMap[4] = {
    offsetof(DelayDSP_Line,line_start),
    offsetof(DelayDSP_Line,line_end),
    offsetof(DelayDSP_Line,feedback),
    offsetof(DelayDSP_Line,input_filter),
};

static uint32_t DelayPresetRegMap[8] = {
    offsetof(DelayDSP,preset_set),
    offsetof(DelayDSP,preset_time1),
    offsetof(DelayDSP,preset_time2),
    offsetof(DelayDSP,preset_feedback),
    offsetof(DelayDSP,preset_vol),
    offsetof(DelayDSP,preset_filter),
    offsetof(DelayDSP,preset_set),
    offsetof(DelayDSP,preset_set),
};

void DelayDSP_write(DelayDSP *c, uint16_t addr, uint16_t data)
{
    if(addr < 0x20)
    {
        *(uint16_t*)((void*)&c->line[addr/4]+DelayRegMap[addr%4]) = data;
        c->line[addr/4].pos = c->line[addr/4].line_start;
    }
    else if(addr < 0x28)
    {
        *(uint16_t*)((void*)c+DelayPresetRegMap[addr%8]) = data;
        DelayDSP_set_preset(c);
    }
}

uint16_t DelayDSP_read(DelayDSP *c, uint16_t addr)
{
    if(addr < 0x20)
        return *(uint16_t*)((void*)&c->line[addr/4]+DelayRegMap[addr%4]);
    else if(addr < 0x28)
        return *(uint16_t*)((void*)c+DelayPresetRegMap[addr%8]);
    else
        return 0;
}

static struct {
    uint8_t time_base[4];
    uint8_t feedback[4];
    uint8_t volume[4];
    uint8_t filter[4];
} PresetArray[] = {
    { // 0
        { 0, 0, 0, 0 }, // Delay time, should be prime numbers. total should be < 128
        { 0, 0, 0, 0 }, // Delay feedback
        { 0, 0, 0, 0 }, // Output volume
        { 0, 0, 0, 0 }, // Output filter cutoff
    },
    { // 1 room
        {   7, 11, 13, 17 }, //=48
        { 245,230,225,220 },
        {  25, 25, 25, 25 },
        {  50,100,150,200 },
    },
    { // 2
        {  11, 13, 17, 19 }, //=60
        { 230,230,235,240 },
        {  35, 35, 33, 30 },
        { 200,180,160,140 },
    },
    { // 3
        {  13, 17, 19, 23 }, //=72
        { 230,230,230,235 },
        {  40, 40, 40, 40 },
        {  20, 30, 40, 40 },
    },
    { // 4 hall
        {  13, 19, 23, 31 }, //=86
        { 250,245,245,245 },
        {  20, 20, 25, 30 },
        { 200,190, 60, 30 },
    },
    { // 5 hall 2
        {  11, 19, 29, 37 }, //=96
        { 245,230,230,240 },
        {  20, 20, 35, 45 },
        {  50,100, 40, 10 },
    },
    { // 6 nice (use full buffer)
        {  11, 17, 47, 53 }, //=128
        { 235,230,175,160 },
        {  50, 45, 40, 35 },
        {  25, 75,150,200 },
    },
    { // 7 simple feedback delay
        { 127,  0,  0,  0 },
        { 150,  0,  0,  0 },
        { 100,  0,  0,  0 },
        {  50,  0,  0,  0 },
    },
};

void DelayDSP_set_preset(DelayDSP *c)
{
    int i;
    int preset = c->preset_set;
    uint16_t addr = 0;

    for(i=0;i<DELAYDSP_LINES;i++)
    {
        uint16_t delaytime = PresetArray[preset].time_base[i/2] * ((i&1) ? c->preset_time1+c->preset_time2 : c->preset_time1);
        uint16_t filter = c->preset_filter * PresetArray[preset].filter[i/2];
        uint16_t feedback = c->preset_feedback * PresetArray[preset].feedback[i/2];
        uint16_t volume = c->preset_vol * PresetArray[preset].volume[i/2];
        c->line[i].line_start = addr;
        c->line[i].line_end = delaytime ? addr+delaytime-1 : addr;
        addr += delaytime;
        c->line[i].input_filter = delaytime ? filter : 0;
        c->line[i].feedback = delaytime ? (feedback & 0xff00) + (volume>>8) : 0;
        c->line[i].pos = c->line[i].line_start;
    }

}
