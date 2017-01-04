/*
    Audio functions
*/

#include "stdint.h"
#include "stdio.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"

#include "qp.h"
#include "audio.h"
#include "lib/vgm.h"

void QP_AudioCallback(void* data,Uint8* astream,int len)
{
    QP_AudioCallbackData* S = (QP_AudioCallbackData*)data;
    float* stream = (float*)astream;

    int i,j;
    float ChipOut[4] = {0,0,0,0};

    int updatemode = S->UpdateRequest;

    double ChipDelta = (double)DriverGetChipRate()/S->SampleRate;
    double DriverDelta = (double)DriverGetTickRate()/S->SampleRate;

    if(S->FastForward)
        DriverDelta *= 32;

    for(i=0;i<S->SampleCount;i++)
    {
        if(updatemode & QPAUDIO_DRV_PLAY)
        {
            S->DriverUpdate += DriverDelta;
            while(S->DriverUpdate > 1)
            {
                DriverUpdateTick();
                //Q_UpdateTick(S->QDrv);

                if(Game->VgmLog)
                {
                    vgm_delay(441000/120);
                }
                S->DriverUpdate-=1;

                GameDoUpdate(Game);
            }
        }
        if(updatemode & QPAUDIO_CHIP_PLAY)
        {
            S->ChipUpdate += ChipDelta;
            while(S->ChipUpdate > 1)
            {
                //C352_update(&S->QDrv->Chip);
                DriverUpdateChip();
                S->ChipUpdate-=1;
            }

            DriverSampleChip(ChipOut,S->MuteRear ? 2 : 4);
            //ChipOut[0] = S->QDrv->Chip.out[0] / 268435456;
            //ChipOut[1] = S->QDrv->Chip.out[1] / 268435456;
            //ChipOut[2] = S->MuteRear ? 0 : S->QDrv->Chip.out[2] / 268435456;
            //ChipOut[3] = S->MuteRear ? 0 : S->QDrv->Chip.out[3] / 268435456;

            //printf("%08x %08x %08x %08x\n",ChipOut[0],ChipOut[1],ChipOut[2],ChipOut[3]);

        }
        if(~updatemode & QPAUDIO_MUTE)
        {
            if(S->OutChannels==1)
                *stream = S->Gain*(ChipOut[0]+ChipOut[1]+ChipOut[2]+ChipOut[3]);
            else if(S->OutChannels==2)
            {
                stream[0] = S->Gain*(ChipOut[0]+ChipOut[2]);
                stream[1] = S->Gain*(ChipOut[1]+ChipOut[3]);
            }
            else if(S->OutChannels==4)
            {
                stream[0] = S->Gain*ChipOut[0];
                stream[1] = S->Gain*ChipOut[1];
                stream[2] = S->Gain*ChipOut[2];
                stream[3] = S->Gain*ChipOut[3];
            }
            else
            {
                // unlikely...
                for(j=0;j<S->OutChannels;j++)
                    stream[j] = ChipOut[0]+ChipOut[1]+ChipOut[2]+ChipOut[3];
            }
        }
        else
        {
            for(j=0;j<S->OutChannels;j++)
                stream[j] = 0;
        }

        stream += S->OutChannels;
    }

    if(S->FileLogging)
    {
        fwrite(astream,S->OutChannels*4,S->SampleCount,S->logfile);
        S->LogSamples += S->SampleCount;
    }

}

void QP_AudioInit(QP_Audio* audio,Q_State* driver,int SampleRate,int SampleCount,char *AudioDevice)
{
    audio->Enabled = 0;
    //audio->state.QDrv = driver;
    //audio->state.SampleRate = SampleRate;
    audio->state.ChipUpdate = 0;
    audio->state.DriverUpdate=0;
    audio->state.MuteRear=0;
    audio->state.Gain=2.0;
    audio->state.FastForward=0;
    audio->state.FileLogging=0;
    audio->state.LogSamples=0;

    SDL_AudioSpec req;
    SDL_zero(req);
    req.callback = QP_AudioCallback;
    req.channels = 4;
    req.freq = SampleRate;
    req.format = AUDIO_F32;
    req.samples = SampleCount; // risky.
    req.userdata = &audio->state;
    audio->dev = SDL_OpenAudioDevice(AudioDevice,0,&req,&audio->as,SDL_AUDIO_ALLOW_CHANNELS_CHANGE|SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

    if(audio->dev)
    {
        printf("got channels=%d, rate=%d\n", audio->as.channels, audio->as.freq );
        audio->state.OutChannels = audio->as.channels;
        audio->state.SampleRate = audio->as.freq;
        audio->state.SampleCount = audio->as.samples;
        audio->Initialized=1;
    }
    else
    {
        printf("Audio init failed: %s\n",SDL_GetError());
        audio->Initialized=0;
    }
}

void QP_AudioClose(QP_Audio* audio)
{
    if(!audio->Initialized)
        return;
    audio->Initialized=0;
    SDL_CloseAudioDevice(audio->dev);
}

void QP_AudioSetPause(QP_Audio* audio,int pause)
{
    if(!audio->Initialized)
        return;
    audio->Enabled=pause;
    SDL_PauseAudioDevice(audio->dev,pause);
}

void QP_AudioTogglePause(QP_Audio* audio)
{
    if(!audio->Initialized)
        return;
    audio->Enabled=audio->Enabled^1;
    SDL_PauseAudioDevice(audio->dev,audio->Enabled);
}

int QP_AudioWavOpen(QP_Audio* audio, char* filename)
{
    audio->state.logfile = NULL;
    audio->state.logfile = fopen(filename,"wb");
    audio->state.LogSamples=0;
    audio->state.FileLogging = 0;
    if(!audio->state.logfile)
        return -1;
    audio->state.FileLogging = 1;

    int i;
    for(i=0;i<58;i++)
    {
        putc(0,audio->state.logfile);
    }

    return 0;
}

void QP_AudioWavClose(QP_Audio* audio)
{
    audio->state.FileLogging=0;
    FILE* f = audio->state.logfile;
    if(!f)
        return;

    uint32_t a;

    uint32_t c = audio->state.OutChannels;
    uint32_t b = audio->state.SampleRate;
    uint32_t d = 4; // bytes per sample

    uint32_t samplecount = audio->state.LogSamples * c;
    uint32_t chunksize = samplecount * d;
    uint32_t chunksize2 = chunksize+50;

    fseek(f,0,SEEK_SET);
    fwrite("RIFF",4,1,f);           // ms id
    fwrite(&chunksize2,4,1,f);      // file size - 8

    fwrite("WAVE",4,1,f);           // Format id

    fwrite("fmt ",4,1,f);           // chunk id
    a=18;                           // chunk size
    fwrite(&a,4,1,f);
    a=3;                            // audio format (Float)
    fwrite(&a,2,1,f);
    fwrite(&c,2,1,f);               // amount of channels
    fwrite(&b,4,1,f);               // sample rate
    a=d*c*b;                        // bytes per second
    fwrite(&a,4,1,f);
    a=d*c;                          // bytes per sample
    fwrite(&a,2,1,f);
    a=d*8;                          // bits per sample
    fwrite(&a,2,1,f);
    a=0;                            // extension
    fwrite(&a,2,1,f);

    fwrite("fact",4,1,f);           // chunk id
    a=4;                            // chunk size
    fwrite(&a,4,1,f);
    fwrite(&samplecount,4,1,f);     // sample count

    fwrite("data",4,1,f);           // chunk id
    fwrite(&chunksize,4,1,f);       // data size

    fclose(audio->state.logfile);
}
