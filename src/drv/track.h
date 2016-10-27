#ifndef TRACK_H_INCLUDED
#define TRACK_H_INCLUDED

// Call 0x06
void Q_TrackInit(Q_State *Q,int TrackNo);
// Call 0x16
void Q_TrackStop(Q_State *Q,int TrackNo);
// Call 0x0a
void Q_TrackUpdate(Q_State *Q,int TrackNo);

// called by Q_TrackStop and track command 0x15.
void Q_TrackDisable(Q_State *Q,int TrackNo);

// Call 0x28
void Q_ChannelClear(Q_State *Q,int TrackNo,int ChannelNo);

// Call 0x14 - call Q_ChannelGetAlloc and Q_ChannelSetVoiceAlloc first.
// this enables the channel.
void Q_ChannelEnable(Q_State *Q,int TrackNo,int ChannelNo);

// calculate track volume including fadeout and attenuations.
void Q_TrackCalcVolume(Q_State *Q,int TrackNo);

// track commands
typedef void (*Q_TrackCommand)(Q_State*,int,Q_Track*,uint32_t*,uint8_t);

// callback for channel write commands
typedef void (*Q_WriteCallback)(Q_State*,int,Q_Track*,uint32_t*,int,int,uint16_t);

Q_TrackCommand Q_TrackCommandTable[0x40];

#endif // TRACK_H_INCLUDED
