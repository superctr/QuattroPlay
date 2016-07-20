#ifndef VOICE_H_INCLUDED
#define VOICE_H_INCLUDED

// Call 0x08 - Allocate a channel to a voice and disables previously allocated channel on voice.
void Q_VoiceSetChannel(Q_State *Q,int VoiceNo,int TrackNo,int ChannelNo);
// Call 0x12 - Clears channel allocation on voice.
void Q_VoiceClearChannel(Q_State *Q,int VoiceNo);

// Call 0x0e - returns highest priority for the voice
// if ch != NULL, it stores the location of Q_Channel with highest priority
uint16_t Q_VoiceGetPriority(Q_State *Q,int VoiceNo,int* TrackNo,int* ChannelNo);
// Call 0x10 - sets priority for a channel on specified voice
void Q_VoiceSetPriority(Q_State *Q,int VoiceNo,int TrackNo,int ChannelNo,int Priority);

// Call 0x24 - process an event
void Q_VoiceProcessEvent(Q_State *Q,int VoiceNo,Q_Voice* V,Q_VoiceEvent *E);

// Call 0x18 - executes key on
void Q_VoiceKeyOn(Q_State *Q,int VoiceNo,Q_Voice* V);

// Call 0x0c - voice update
void Q_VoiceUpdate(Q_State *Q,int VoiceNo,Q_Voice* V);

// ---------------------------------------------------------------------------
// Volume envelopes

//
void Q_VoiceEnvSet(Q_State *Q,int VoiceNo,Q_Voice* V);
// 0x76fc
void Q_VoiceEnvRead(Q_State *Q,int VoiceNo,Q_Voice* V);
// Call 0x1a
void Q_VoiceEnvUpdate(Q_State *Q,int VoiceNo,Q_Voice* V);

// ---------------------------------------------------------------------------
// Pan-related

// Set pan mode & variables
void Q_VoicePanSet(Q_State *Q,int VoiceNo,Q_Voice* V);

// Set panning and write volume to chip
void Q_VoicePanSetVolume(Q_State *Q,int VoiceNo,Q_Voice *V,int8_t pan);
// Write volume to chip
void Q_VoiceSetVolume(Q_State *Q,int VoiceNo,Q_Voice *V, uint16_t VolumeF, uint16_t VolumeR);
// update pan envelope
void Q_VoicePanEnvUpdate(Q_State *Q,int VoiceNo,Q_Voice *V);
void Q_VoicePanEnvRead(Q_State *Q,int VoiceNo,Q_Voice *V);


// call 0x1c - pan update
void Q_VoicePanUpdate(Q_State *Q,int VoiceNo,Q_Voice *V);
// call 0x2c - convert panpot value to volume
void Q_VoicePanConvert(Q_State *Q,int8_t pan,uint16_t *VolumeF,uint16_t *VolumeR);
// call 0x2d - convert position value to volume
void Q_VoicePosConvert(Q_State *Q,int8_t xpos,int8_t ypos,uint16_t *VolumeF,uint16_t *VolumeR);

// ---------------------------------------------------------------------------
// Pitch-related

// Set initial params for pitch envelope
void Q_VoicePitchEnvSet(Q_State *Q,int VoiceNo,Q_Voice *V);
void Q_VoicePitchEnvSetMod(Q_State *Q,int VoiceNo,Q_Voice *V);

// call 0x20 - updates pitch including portamento
void Q_VoicePortaUpdate(Q_State *Q,int VoiceNo,Q_Voice *V);
// call 0x1e - updates pitch envelope...
void Q_VoicePitchEnvUpdate(Q_State *Q,int VoiceNo,Q_Voice *V);

// ---------------------------------------------------------------------------
// Lfo-related

// 0x676c
void Q_VoiceLfoSet(Q_State *Q,int VoiceNo,Q_Voice *V);
// call 0x22
void Q_VoiceLfoUpdate(Q_State *Q,int VoiceNo,Q_Voice *V);

uint8_t Q_VoiceLfoGetRandom(Q_State *Q);
uint16_t Q_VoiceLfoUpdateEnv(uint16_t a,uint16_t b,uint16_t delta);
void Q_VoiceLfoSetMod(Q_State *Q,int VoiceNo,Q_Voice *V,int8_t offset);

// ---------------------------------------------------------------------------
// Voice-related

void Q_WaveSet(Q_State* Q,int VoiceNo,Q_Voice* V,uint16_t WaveNo);
void Q_WaveReset(Q_State* Q,int VoiceNo,Q_Voice* V);

// Call 0x2a
void Q_WaveLinkUpdate(Q_State*Q,int VoiceNo,Q_Voice* V);

#endif // VOICE_H_INCLUDED
