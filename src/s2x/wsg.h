#ifndef WSG_H_INCLUDED
#define WSG_H_INCLUDED

enum S2X_WSGHeaderId {
    S2X_WSG_HEADER_TRACKREQ = 0,
    S2X_WSG_HEADER_TRACKTYPE = 2,
    S2X_WSG_HEADER_TRACK = 4,
    S2X_WSG_HEADER_TRACKWORK = 6,
    S2X_WSG_HEADER_PITCHTAB = 8,
    S2X_WSG_HEADER_ENVELOPE = 10,
    S2X_WSG_HEADER_WAVEFORM = 12
};

uint16_t S2X_WSGReadHeader(S2X_State *S,enum S2X_WSGHeaderId id);
void S2X_WSGLoadWave(S2X_State *S);
void S2X_WSGTrackUpdate(S2X_State *S,int TrackNo,S2X_Track *T);
void S2X_WSGChannelUpdate(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo);
void S2X_WSGChannelStart(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo,uint16_t pos,uint8_t flag);

#endif // WSG_H_INCLUDED
