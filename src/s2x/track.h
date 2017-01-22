#ifndef S2X_TRACK_H_INCLUDED
#define S2X_TRACK_H_INCLUDED

void S2X_TrackInit(S2X_State* S, int TrackNo);
void S2X_TrackStop(S2X_State* S,int TrackNo);
void S2X_TrackUpdate(S2X_State* S,int TrackNo);
void S2X_TrackCalcVolume(S2X_State* S,int TrackNo);
void S2X_TrackDisable(S2X_State *S,int TrackNo);
void S2X_ChannelClear(S2X_State *S,int TrackNo,int ChannelNo);

#define S2X_MAX_TRKCMD 0x25

typedef void (*S2X_TrackCommand)(S2X_State*,int,S2X_Track*,uint8_t);

struct S2X_TrackCommandEntry{
    int type; // this will only be used for the pattern parser.
    S2X_TrackCommand cmd;
};
// command entry types
// positive : skip <n> bytes
//  0 : end of data
// -1 : channel command
// -2 : key on
// -3 : portamento
// -4 : conditional jump
// -5 : subroutine
// -6 : jump
// -7 : repeat
// -8 : loop
// -9 : return (or end of data)

struct S2X_TrackCommandEntry* S2X_TrackCommandTable[S2X_TYPE_MAX];
//struct S2X_TrackCommandEntry S2X_S2TrackCommandTable[S2X_MAX_TRKCMD];
//struct S2X_TrackCommandEntry S2X_S1TrackCommandTable[S2X_MAX_TRKCMD];
//struct S2X_TrackCommandEntry S2X_NATrackCommandTable[S2X_MAX_TRKCMD];

#endif // S2X_TRACK_H_INCLUDED
