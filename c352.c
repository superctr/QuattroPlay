/*
    C352 chip emulator for QuattroPlay
    written by ctr with help from MAME source code (written by R.Belmont)

    perhaps internal registers are readable at 0x100-0x1ff
*/
#include <stdio.h>
#include <stdlib.h>
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "math.h"
#include "c352.h"


void C352_generate_mulaw(C352 *c)
{
    int i;
    double x_max = 32752.0;
    double y_max = 127.0;
    double u = 10.0;

    // generate mulaw table for mulaw format samples
    for (i = 0; i < 256; i++)
    {
        double y = (double) (i & 0x7f);
        double x = (exp (y / y_max * log (1.0 + u)) - 1.0) * x_max / u;

        if (i & 0x80)
            x = -x;
        c->mulaw[i] = (short)x;
    }
}

int C352_init(C352 *c, uint32_t clk)
{
    c->mute_mask=0;
    c->rate = clk/288;

    memset(c->v,0,sizeof(C352_Voice)*C352_VOICES);
    memset(c->out,0,4*sizeof(int16_t));

    c->control1 = 0;
    c->control2 = 0;
    c->random = 0x1234;

    C352_generate_mulaw(c);

    return c->rate;
}

uint16_t C352RegMap[8] = {
    offsetof(C352_Voice,vol_f),
    offsetof(C352_Voice,vol_r),
    offsetof(C352_Voice,freq),
    offsetof(C352_Voice,flags),
    offsetof(C352_Voice,wave_bank),
    offsetof(C352_Voice,wave_start),
    offsetof(C352_Voice,wave_end),
    offsetof(C352_Voice,wave_loop),
};

void C352_write(C352 *c, uint16_t addr, uint16_t data)
{
    int i;

    if(addr < 0x100)
        *(uint16_t*)((void*)&c->v[addr/8]+C352RegMap[addr%8]) = data;
    else if(addr == 0x200)
        c->control1 = data;
    else if(addr == 0x201)
        c->control2 = data;
    else if(addr == 0x202) // execute keyons/keyoffs
    {
        for(i=0;i<C352_VOICES;i++)
        {
            if((c->v[i].flags & C352_FLG_KEYON))
            {
                c->v[i].pos = (c->v[i].wave_bank<<16) | c->v[i].wave_start;

				c->v[i].sample = 0;
                c->v[i].last_sample = 0;
				c->v[i].counter = 0x10000;

                c->v[i].flags |= C352_FLG_BUSY;
                c->v[i].flags &= ~(C352_FLG_KEYON|C352_FLG_LOOPHIST);
            }
            else if(c->v[i].flags & C352_FLG_KEYOFF)
            {
                c->v[i].flags &= ~(C352_FLG_BUSY|C352_FLG_KEYOFF);
            }
        }
    }
}

uint16_t C352_read(C352 *c, uint16_t addr)
{
    if(addr < 0x100)
        return *(uint16_t*)((void*)&c->v[addr/8]+C352RegMap[addr%8]);
    else
        return 0;
}


void C352_fetch_sample(C352 *c, int i)
{
    C352_Voice *v = &c->v[i];
	v->last_sample = v->sample;

    if(v->flags & C352_FLG_NOISE)
    {
        c->random = (c->random>>1) ^ ((-(c->random&1)) & 0xfff6);
        // don't know what is most accurate here...
        v->sample = c->random; // (c->random&4) ? 0xc000 : 0x3fff;
	}
	else
	{
		int8_t s;

		s = (int8_t)c->wave[v->pos&c->wave_mask];

		if(v->flags & C352_FLG_MULAW)
			v->sample = c->mulaw[(uint8_t)s];
		else
			v->sample = s<<8;

		uint16_t pos = v->pos&0xffff;

        if((v->flags & C352_FLG_LOOP) && v->flags & C352_FLG_REVERSE)
        {
			// backwards>forwards
            if((v->flags & C352_FLG_LDIR) && pos == v->wave_loop)
                v->flags &= ~C352_FLG_LDIR;
			// forwards>backwards
            else if(!(v->flags & C352_FLG_LDIR) && pos == v->wave_end)
                v->flags |= C352_FLG_LDIR;

			v->pos += (v->flags&C352_FLG_LDIR) ? -1 : 1;
        }
        else if(pos == v->wave_end)
        {
            if((v->flags & C352_FLG_LINK) && (v->flags & C352_FLG_LOOP))
            {
                v->pos = (v->wave_start<<16) | v->wave_loop;
                v->flags |= C352_FLG_LOOPHIST;
            }
            else if(v->flags & C352_FLG_LOOP)
            {
                v->pos = (v->pos&0xff0000) | v->wave_loop;
                v->flags |= C352_FLG_LOOPHIST;
            }
            else
            {
                v->flags |= C352_FLG_KEYOFF;
                v->flags &= ~C352_FLG_BUSY;
				v->sample=0;
            }
        }
		else
		{
			v->pos += (v->flags&C352_FLG_REVERSE) ? -1 : 1;
		}
	}
}

int16_t C352_update_voice(C352 *c, int i)
{

    C352_Voice *v = &c->v[i];

    if((v->flags & C352_FLG_BUSY) == 0)
        return 0;

	v->counter += v->freq;

	if(v->counter > 0x10000)
	{
		v->counter &= 0xffff;
		C352_fetch_sample(c,i);
	}

	int32_t temp = v->sample;
	// Interpolate samples
    if((v->flags & C352_FLG_FILTER) == 0)
         temp = v->last_sample + (v->counter*(v->sample-v->last_sample)>>16);

    return temp;
}

void C352_update(C352 *c)
{
    int i;
    int16_t s;

    c->out[0]=c->out[1]=c->out[2]=c->out[3]=0;

    for(i=0;i<C352_VOICES;i++)
    {
        s = C352_update_voice(c,i);

        if(!(c->mute_mask & 1<<i))
        {
            // Left
            c->out[0] += (c->v[i].flags & C352_FLG_PHASEFL) ? -s * (c->v[i].vol_f>>8)
                                                            :  s * (c->v[i].vol_f>>8);
            c->out[2] += (c->v[i].flags & C352_FLG_PHASERL) ? -s * (c->v[i].vol_r>>8)
                                                            :  s * (c->v[i].vol_r>>8);

            // Right
            c->out[1] += (c->v[i].flags & C352_FLG_PHASEFR) ? -s * (c->v[i].vol_f&0xff)
                                                            :  s * (c->v[i].vol_f&0xff);
            c->out[3] += (c->v[i].flags & C352_FLG_PHASEFR) ? -s * (c->v[i].vol_r&0xff)
                                                            :  s * (c->v[i].vol_r&0xff);
        }
    }
}
