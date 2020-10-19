// license:GPL-2.0+
// copyright-holders:Jarek Burczynski,Ernesto Corvi

// Ported from MAME C++ sources by ctr
// I removed some comments in order to 'slim' down the file.
// if you want to know more about this chip, please look at the original MAME sources.

#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#include "ym2151.h"

void YM2151_init(YM2151* ym,int clk)
{
    ym->rate = clk/64;
    memset(&ym->chip, 0, sizeof(ym->chip));
    ym->chip.mute_mask=0;

    YM2151_reset(ym);
}

void YM2151_reset(YM2151* ym)
{
    int i;
    ym->chip.ic = 1;
    /* initialize hardware registers */
    for (i=0; i<32; i++)
    {
        OPM_Clock(&ym->chip);
    }
    ym->chip.ic = 0;

    ym->out[0] = ym->out[1] = ym->out[2] = ym->out[3] = 0;
}


void YM2151_update(YM2151* ym)
{
    int32_t output[2];
    ym->chip.mute_mask = ym->mute_mask;
    do
    {
        OPM_Clock(&ym->chip);
    }
    while(ym->chip.cycles != 14);

    // last samples, used for interpolation
    ym->out[2] = ym->out[0];
    ym->out[3] = ym->out[1];

    OPM_GetSample(&ym->chip, output, NULL, NULL, NULL);
    ym->out[0] = output[0]/32768.0;
    ym->out[1] = output[1]/32768.0;
}

void YM2151_write_reg(YM2151* ym,int r, int v)
{
    uint32_t cyc = ym->chip.cycles;
    uint32_t counter = 100;
    while(YM2151_busy(ym) && counter--);
    {
        OPM_Clock(&ym->chip);
    }

    OPM_Write(&ym->chip, 0, r);
    OPM_Clock(&ym->chip);
    OPM_Clock(&ym->chip);
    OPM_Clock(&ym->chip);

    OPM_Write(&ym->chip, 1, v);
}

int YM2151_busy(YM2151* ym)
{
    return (ym->chip.write_busy | ym->chip.write_d_en | ym->chip.write_d);
}
