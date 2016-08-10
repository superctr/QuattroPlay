#ifndef VGM_H_INCLUDED
#define VGM_H_INCLUDED

#include "stdint.h"

// samplerom, samplelen, rom_offset
//void vgm_open(char* fname, uint8_t* datablock, uint32_t dbsize, uint32_t startoffset);
void vgm_open(char* fname);
void vgm_write(uint8_t command, uint8_t port, uint16_t reg, uint16_t value);
void vgm_delay(uint32_t delay);
void vgm_setloop();
void vgm_loop();
void vgm_stop();
void vgm_write_tag(char* gamename,int songid);
void vgm_close();
void vgm_poke32(int32_t offset, uint32_t d);
void vgm_datablock(uint8_t dbtype, uint32_t dbsize, uint8_t* datablock, uint32_t maxsize, uint32_t mask, int32_t flags);

#endif // VGM_H_INCLUDED
