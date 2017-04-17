#ifndef S2X_TRACK_H_INCLUDED
#define S2X_TRACK_H_INCLUDED

void S2X_TrackInit(S2X_State* S, int TrackNo);
void S2X_TrackStop(S2X_State* S,int TrackNo);
void S2X_TrackUpdate(S2X_State* S,int TrackNo);
void S2X_TrackCalcVolume(S2X_State* S,int TrackNo);
void S2X_TrackDisable(S2X_State *S,int TrackNo);
void S2X_ChannelInit(S2X_State* S,S2X_Track* T,int TrackNo,int start,uint8_t mask);
void S2X_ChannelClear(S2X_State *S,int TrackNo,int ChannelNo);

#define S2X_MAX_TRKCMD 0x25

typedef void (*S2X_TrackCommand)(S2X_State*,int,S2X_Track*,uint8_t,int8_t);

struct S2X_TrackCommandEntry{
    int type; // this will only be used for the pattern parser.
    int param;
    S2X_TrackCommand cmd;
};
// command entry types
enum {
    S2X_CMD_END = 0,
    S2X_CMD_CHN = -1,
    S2X_CMD_FRQ = -2,
    S2X_CMD_WAV = -3,
    S2X_CMD_TRS = -4,
    S2X_CMD_CJUMP = -5,
    S2X_CMD_CALL = -6,
    S2X_CMD_JUMP = -7,
    S2X_CMD_REPT = -8,
    S2X_CMD_LOOP = -9,
    S2X_CMD_RET = -10,
    S2X_CMD_CALL86 = -11, // Special call/jump cmds for system86
    S2X_CMD_JUMP86 = -12, // they use a song ID as arguments...
    S2X_CMD_EMPTY = -13, // empty row
};
struct S2X_TrackCommandEntry* S2X_TrackCommandTable[S2X_TYPE_MAX];
//struct S2X_TrackCommandEntry S2X_S2TrackCommandTable[S2X_MAX_TRKCMD];
//struct S2X_TrackCommandEntry S2X_S1TrackCommandTable[S2X_MAX_TRKCMD];
//struct S2X_TrackCommandEntry S2X_NATrackCommandTable[S2X_MAX_TRKCMD];

#endif // S2X_TRACK_H_INCLUDED
