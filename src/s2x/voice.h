#ifndef S2X_VOICE_H_INCLUDED
#define S2X_VOICE_H_INCLUDED

void S2X_VoiceSetChannel(S2X_State *S,int VoiceNo,int TrackNo,int ChannelNo);
void S2X_VoiceClearChannel(S2X_State *S,int VoiceNo);
uint16_t S2X_VoiceGetPriority(S2X_State *S,int VoiceNo,int* TrackNo,int* ChannelNo);
void S2X_VoiceSetPriority(S2X_State *S,int VoiceNo,int TrackNo,int ChannelNo,int Priority);

int S2X_SetVoiceType(S2X_State *S,int VoiceNo,int VoiceType,int Count);
int S2X_GetVoiceType(S2X_State *S,int VoiceNo);
int S2X_GetVoiceIndex(S2X_State *S,int VoiceNo,int VoiceType);

void S2X_VoiceClear(S2X_State *S,int VoiceNo);
void S2X_VoiceCommand(S2X_State *S,S2X_Channel *C,int Command,uint8_t Data);
void S2X_VoiceUpdate(S2X_State *S,int VoiceNo);

void S2X_VoicePitchEnvSet(S2X_State *S,struct S2X_Pitch *P);
void S2X_VoicePitchEnvUpdate(S2X_State *S,struct S2X_Pitch *P);
void S2X_VoicePitchEnvSetMod(S2X_State *S,struct S2X_Pitch *P);
void S2X_VoicePitchEnvSetVol(S2X_State *S,struct S2X_Pitch *P);
void S2X_VoicePitchUpdate(S2X_State *S,struct S2X_Pitch *P);

void S2X_PCMClear(S2X_State *S,S2X_PCMVoice *V,int VoiceNo);
void S2X_PCMCommand(S2X_State *S,S2X_Channel *C,S2X_PCMVoice *V);
void S2X_PCMUpdate(S2X_State *S,S2X_PCMVoice *V);

void S2X_PCMAdsrSet(S2X_State *S,S2X_PCMVoice *V,uint8_t EnvNo);

void S2X_FMClear(S2X_State *S,S2X_FMVoice *V,int VoiceNo);
void S2X_FMCommand(S2X_State *S,S2X_Channel *C,S2X_FMVoice *V);
void S2X_FMUpdate(S2X_State *S,S2X_FMVoice *V);

void S2X_FMKeyOff(S2X_State *S,S2X_FMVoice *V);
void S2X_FMSetIns(S2X_State *S,S2X_FMVoice *V,int InsNo);
void S2X_FMSetVol(S2X_State *S,S2X_FMVoice *V);
void S2X_FMWriteVol(S2X_State *S,S2X_FMVoice *V,int Attenuation);
void S2X_FMSetLfo(S2X_State *S,S2X_FMVoice *V,int LfoNo);

void S2X_PlayPercussion(S2X_State *S,int VoiceNo,int BaseAddr,int WaveNo,int VolMod);

#endif // S2X_VOICE_H_INCLUDED
