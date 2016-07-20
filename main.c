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

#ifdef DEBUG
    int bootsong = 0;
#else
    int bootsong = 1;
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

    if(argc>1)
        strcpy(gamename, argv[1]);

    if(LoadGame(QDrv,gamename))
    {
        SDL_Quit();
        return -1;
    }

    QDrv->BootSong=bootsong;
    Q_Init(QDrv);

    QPAudio_Init(Audio,QDrv,QDrv->Chip.rate,1024,NULL);

    //..
    ui_main();
    QPAudio_Close(Audio);
    SDL_Quit();

    UnloadGame(QDrv);

    free(QDrv);
    return 0;

}

