#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "stdio.h"

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

} QP_AudioCallbackData;

typedef struct {

    SDL_AudioSpec as;
    SDL_AudioDeviceID dev;
    QP_AudioCallbackData state;

    int Initialized;
    int Enabled;

} QP_Audio;

void QP_AudioInit(QP_Audio* audio,Q_State* driver,int SampleRate,int SampleCount,char *AudioDevice);
void QP_AudioClose(QP_Audio* audio);
void QP_AudioSetPause(QP_Audio* audio,int pause);
void QP_AudioTogglePause(QP_Audio* audio);

int  QP_AudioWavOpen(QP_Audio* audio, char* filename);
void QP_AudioWavClose(QP_Audio* audio);
#endif // AUDIO_H_INCLUDED
