#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "vgm.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "ui.h"
#include "ini.h"
#include "loader.h"
#include "drv/quattro.h"


int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER);

//    static char inipath[128];
//    static char wavepath[128];
//    static char datapath[128];
    static char gamename[128];
    static char filename[FILENAME_MAX];

    int wavlog = 0;
    int vgmlog = 0;
    int autoplay_song = -1;
#ifdef DEBUG
    bootsong = 0;
#else
    bootsong = 1;
#endif

    QDrv = (Q_State*)malloc(sizeof(Q_State));
    memset(QDrv,0,sizeof(Q_State));

    audio_t audio_instance;
    Audio = &audio_instance;
/*
    QP_PlaylistState playlist_instance;
    pl = &playlist_instance;

    pl->MaxEntries=0;
    pl->CurrEntry=0;
    pl->PlayState = QP_PLAYSTATE_IDLE;
    pl->CommandPos=0;
*/
    muterear=0;
    gain=32.0;

    inifile_t initest;
    if(!ini_open("quattroplay.ini",&initest))
    {
        while(!ini_readnext(&initest))
        {
            //printf("'%s'.'%s' = '%s'\n",initest.section,initest.key,initest.value);

            if(!strcmp(initest.section,"config"))
            {
                if(!strcmp(initest.key,"inipath"))
                    strcpy(L_IniPath,initest.value);
                else if(!strcmp(initest.key,"wavepath"))
                    strcpy(L_WavePath,initest.value);
                else if(!strcmp(initest.key,"datapath"))
                    strcpy(L_DataPath,initest.value);
                else if(!strcmp(initest.key,"gamename"))
                    strcpy(gamename,initest.value);
                else if(!strcmp(initest.key,"gain"))
                    gain = atof(initest.value);
                else if(!strcmp(initest.key,"bootsong"))
                    bootsong = atoi(initest.value);
            }
        }
        ini_close(&initest);
    }
    else
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error","Config file does not exist. Please create one.",NULL);
        SDL_Quit();
        return -1;
    }

    int i, standard_args=0;
    for(i=1;i<argc;i++)
    {
        if((!strcmp(argv[i],"-a") || !strcmp(argv[i],"--autoplay")) && i<argc)
        {
            i++;
            autoplay_song = (int)strtol(argv[i],NULL,0);
        }
        else if(!strcmp(argv[i],"-w") || !strcmp(argv[i],"--wavlog"))
        {
            wavlog=1;
        }
        else if(!strcmp(argv[i],"-v") || !strcmp(argv[i],"--vgmlog"))
        {
            vgmlog=1;
        }
        else
        {
            if(standard_args == 0)
                strcpy(gamename, argv[i]);
            else if(standard_args == 1)
                autoplay_song = (int)strtol(argv[i],NULL,0);
            else
                break;
            standard_args++;
        }

    }
    /*
    if(argc>1)
        strcpy(gamename, argv[1]);
    if(argc>2)
        autoplay_song = strtol(argv[2],NULL,0);
    */
    if(LoadGame(QDrv,gamename))
    {
        SDL_Quit();
        return -1;
    }

    if(vgmlog)
    {
        strcpy(filename,"qp_log.vgm");
        if(autoplay_song >= 0)
        {
            sprintf(filename,"%s_%03x.vgm",gamename,autoplay_song&0x7ff);
        }
        vgm_open(filename);
        vgm_datablock(0x92,0x1000000,QDrv->Chip.wave,0x1000000,QDrv->Chip.wave_mask,0);
        QDrv->Chip.vgm_log = 1;
    }

    QDrv->BootSong=bootsong;
    Q_Init(QDrv);

    QPAudio_Init(Audio,QDrv,QDrv->Chip.rate,1024,NULL);

    if(autoplay_song >= 0)
        QDrv->BootSong=2;

    Audio->state.AutoPlaySong = autoplay_song;

    if(wavlog)
    {
        strcpy(filename,"qp_log.wav");
        if(autoplay_song >= 0)
        {
            sprintf(filename,"%s_%03x.wav",gamename,autoplay_song&0x7ff);
        }
        QPAudio_WavOpen(Audio,filename);
    }

    ui_main();

    if(Audio->state.FileLogging)
    {
        SDL_LockAudioDevice(Audio->dev);
        QPAudio_WavClose(Audio);
        SDL_UnlockAudioDevice(Audio->dev);
    }

    if(QDrv->Chip.vgm_log)
    {
        SDL_LockAudioDevice(Audio->dev);
        QDrv->Chip.vgm_log = 0;
        vgm_poke32(0xdc,QDrv->ChipClock | Audio->state.MuteRear<<31);
        vgm_poke8(0xd6,288/4);
        vgm_stop();
        vgm_write_tag(strlen(L_GameTitle) ? L_GameTitle : gamename,autoplay_song);
        vgm_close();
        SDL_UnlockAudioDevice(Audio->dev);
    }

    QPAudio_Close(Audio);

    SDL_Quit();
    UnloadGame(QDrv);
    free(QDrv);

    return 0;
}

