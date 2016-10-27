#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "drv/quattro.h"

enum {
    QPAUDIO_DRV_PLAY = 1,
    QPAUDIO_CHIP_PLAY = 2,
    QPAUDIO_MUTE = 4,
};
typedef struct {

    Q_State *QDrv;

    // temporary home for this variable until i find a better place.
    int AutoPlaySong;

    int UpdateRequest;

    uint32_t SampleRate;

    int FastForward;

    double ChipUpdate;
    double DriverUpdate;

    float Gain;

    int MuteRear; // set to mute rear channels (for systems that don't have them)
    int OutChannels; // words per sample
    int SampleCount;

    int FileLogging;
    FILE* logfile;
    uint32_t LogSamples;

} audiocb_t;

typedef struct {

    SDL_AudioSpec as;
    SDL_AudioDeviceID dev;
    audiocb_t state;

    int Initialized;
    int Enabled;

} audio_t;

void QPAudio_Init(audio_t* audio,Q_State* driver,int SampleRate,int SampleCount,char *AudioDevice);
void QPAudio_Close(audio_t* audio);
void QPAudio_SetPause(audio_t* audio,int pause);
void QPAudio_TogglePause(audio_t* audio);

int  QPAudio_WavOpen(audio_t* audio, char* filename);
void QPAudio_WavClose(audio_t* audio);
#endif // AUDIO_H_INCLUDED
