#ifndef S2X_TRACK_H_INCLUDED
#define S2X_TRACK_H_INCLUDED

void S2X_TrackInit(S2X_State* S, int TrackNo);
void S2X_TrackStop(S2X_State* S,int TrackNo);
void S2X_TrackUpdate(S2X_State* S,int TrackNo);
void S2X_TrackCalcVolume(S2X_State* S,int TrackNo);
void S2X_TrackDisable(S2X_State *S,int TrackNo);
void S2X_ChannelClear(S2X_State *S,int TrackNo,int ChannelNo);

#endif // S2X_TRACK_H_INCLUDED
