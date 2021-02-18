#ifndef WSG_H_INCLUDED
#define WSG_H_INCLUDED

enum S2X_S1WSGHeaderId {
    S2X_S1WSG_HEADER_TRACKREQ = 0,
    S2X_S1WSG_HEADER_TRACKTYPE = 2,
    S2X_S1WSG_HEADER_TRACK = 4,
    S2X_S1WSG_HEADER_TRACKWORK = 6,
    S2X_S1WSG_HEADER_PITCHTAB = 8,
    S2X_S1WSG_HEADER_ENVELOPE = 10,
    S2X_S1WSG_HEADER_WAVEFORM = 12
};

uint16_t S2X_S1WSGReadHeader(S2X_State *S,enum S2X_S1WSGHeaderId id);
void S2X_S1WSGLoadWave(S2X_State *S);
int S2X_S1WSGTrackStart(S2X_State *S,int TrackNo,S2X_Track *T,int SongNo);
void S2X_S1WSGTrackUpdate(S2X_State *S,int TrackNo,S2X_Track *T);
void S2X_S1WSGChannelUpdate(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo);
void S2X_S1WSGChannelStart(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo,uint16_t pos,uint8_t flag);

enum S2X_S86WSGHeaderId {
    S2X_S86WSG_HEADER_WAVEFORM = 0,
    S2X_S86WSG_HEADER_TRACKWORK = 1,
    S2X_S86WSG_HEADER_TRACK = 2,
    S2X_S86WSG_HEADER_ENVELOPE = 3,
    S2X_S86WSG_HEADER_TRACKREQ = 6,
    S2X_S86WSG_HEADER_TEMPO = 7,
};

uint16_t S2X_S86WSGReadHeader(S2X_State *S,enum S2X_S86WSGHeaderId id);
void S2X_S86WSGLoadWave(S2X_State *S);
int S2X_S86WSGTrackStart(S2X_State *S,int TrackNo,S2X_Track *T,int SongNo);
void S2X_S86WSGTrackUpdate(S2X_State *S,int TrackNo,S2X_Track *T);
void S2X_S86WSGEnvelopeUpdate(S2X_State *S,struct S2X_WSGEnvelope *E,S2X_Channel *C);
void S2X_S86WSGChannelUpdate(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo);
void S2X_S86WSGChannelStart(S2X_State *S,int TrackNo,S2X_Channel *C,int ChannelNo,uint16_t pos);

#endif // WSG_H_INCLUDED
