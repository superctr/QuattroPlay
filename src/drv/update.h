#ifndef UPDATE_H_INCLUDED
#define UPDATE_H_INCLUDED

// call 0x04 - update tracks and execute song requests
void Q_UpdateTracks(Q_State *Q);

// call 0x26 - update all voices
void Q_UpdateVoices(Q_State* Q);

#endif // UPDATE_H_INCLUDED
