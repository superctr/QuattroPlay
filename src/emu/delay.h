#ifndef DELAY_H_INCLUDED
#define DELAY_H_INCLUDED

#include <stdint.h>

#define DELAYDSP_LINES 8

typedef struct {

    uint16_t line_start;
    uint16_t line_end;
    uint16_t feedback;
    uint16_t input_filter;

    uint16_t pos;
    int32_t filter_state;

} DelayDSP_Line;

typedef struct {

    uint16_t config;
    uint16_t time;
    uint16_t time_offset;
    uint16_t feedback;
    uint16_t volume;
    uint16_t filter;

} DelayDSP_Preset;

typedef struct {

    int in_count;
    int enable_count;

    int16_t *input;
    int16_t output[4];

    int16_t delay_memory[0x10000];
    DelayDSP_Line line[DELAYDSP_LINES];

    int last_preset;
    DelayDSP_Preset preset;
} DelayDSP;

int DelayDSP_init(DelayDSP *c);

void DelayDSP_set_input(DelayDSP *c,int16_t *a,int in_count,int enable_count);

void DelayDSP_update(DelayDSP *c);

void DelayDSP_write(DelayDSP *c, uint16_t addr, uint16_t data);
uint16_t DelayDSP_read(DelayDSP *c, uint16_t addr);

void DelayDSP_config_preset(DelayDSP *c);
void DelayDSP_set_preset(DelayDSP *c, int preset);
#endif // DELAY_H_INCLUDED
