/*
    Quattro - data table defines
*/
#ifndef TABLES_H_INCLUDED
#define TABLES_H_INCLUDED

#include "stdint.h"

uint16_t Q_EnvelopeRateTable[0xa0];
uint16_t Q_PitchTable[0x90];
uint16_t Q_LfoWaveTable[0xb0];
uint8_t Q_PanTable[0x40];
uint8_t Q_VolumeTable[0x100];

uint8_t Q_TrackStructMap[0x22];
uint8_t Q_ChannelStructMap[0x20];

#endif // TABLES_H_INCLUDED
