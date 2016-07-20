#ifndef TRACK_CMD_H_INCLUDED
#define TRACK_CMD_H_INCLUDED

// track commands
typedef void (*Q_TrackCommand)(Q_State*,int,Q_Track*,uint32_t*,uint8_t);

// callback for channel write commands
typedef void (*Q_WriteCallback)(Q_State*,int,Q_Track*,uint32_t*,int,int,uint16_t);


Q_TrackCommand Q_TrackCommandTable[0x40];



#endif // TRACK_CMD_H_INCLUDED
