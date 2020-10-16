#ifndef YM2151_H_INCLUDED
#define YM2151_H_INCLUDED

// YM2151 Register names
enum {
    OPM_TEST = 0x01,
    OPM_KEYON = 0x08,
    OPM_LFO_FRQ = 0x18,
    OPM_LFO_DEP = 0x19,
    OPM_LFO_WAV = 0x1b,
    OPM_CH_CONTROL = 0x20,
    OPM_CH_KEYCODE = 0x28,
    OPM_CH_KEYFRAC = 0x30,
    OPM_CH_PMS_AMS = 0x38,
    OPM_OP_DT1_MUL = 0x40,
    OPM_OP_TL      = 0x60,
    OPM_OP_KS_AR   = 0x80,
    OPM_OP_AME_D1R = 0xa0,
    OPM_OP_DT2_D2R = 0xc0,
    OPM_OP_D1L_RR  = 0xe0
};

#include "opm.h"

typedef struct YM2151 YM2151;

struct YM2151
{
	opm_t chip;

    uint32_t mute_mask;
    double out[4];

    int rate;
};

// temp until opm.h exports these functions
void OPM_Clock(opm_t *chip, int32_t *output);
void OPM_Write(opm_t* chip, uint32_t port, uint8_t data);

void YM2151_init(YM2151* ym,int clk);
void YM2151_reset(YM2151* ym);
void YM2151_update(YM2151* ym);
void YM2151_write_reg(YM2151* ym,int r, int v);

int YM2151_busy(YM2151* ym);

#endif // YM2151_H_INCLUDED
