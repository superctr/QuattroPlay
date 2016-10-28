#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "vgm.h"
#include "qp.h"
//#include "SDL2/SDL.h"
//#include "SDL2/SDL_audio.h"
#include "ui/ui.h"
#include "ini.h"
//#include "loader.h"
//#include "drv/quattro.h"


int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER);

//    static char inipath[128];
//    static char wavepath[128];
//    static char datapath[128];
//    static char gamename[128];

    QDrv = (Q_State*)malloc(sizeof(Q_State));
    memset(QDrv,0,sizeof(Q_State));

    Audio = (audio_t*)malloc(sizeof(audio_t));
    memset(Audio,0,sizeof(audio_t));

    Game = (game_t*)malloc(sizeof(game_t));
    memset(Game,0,sizeof(game_t));

    if(!QDrv || !Audio || !Game)
        return -1;

    Game->AutoPlay = -1;
/*
    QP_PlaylistState playlist_instance;
    pl = &playlist_instance;

    pl->MaxEntries=0;
    pl->CurrEntry=0;
    pl->PlayState = QP_PLAYSTATE_IDLE;
    pl->CommandPos=0;
*/
    Game->MuteRear=0;
    Game->BaseGain=32.0;
    Game->AudioBuffer=1024;

    inifile_t initest;
    if(!ini_open("quattroplay.ini",&initest))
    {
        while(!ini_readnext(&initest))
        {
            //printf("'%s'.'%s' = '%s'\n",initest.section,initest.key,initest.value);

            if(!strcmp(initest.section,"config"))
            {
                if(!strcmp(initest.key,"inipath"))
                    strcpy(QP_IniPath,initest.value);
                else if(!strcmp(initest.key,"wavepath"))
                    strcpy(QP_WavePath,initest.value);
                else if(!strcmp(initest.key,"datapath"))
                    strcpy(QP_DataPath,initest.value);
                else if(!strcmp(initest.key,"gamename"))
                    strcpy(Game->Name,initest.value);
                else if(!strcmp(initest.key,"gain"))
                    Game->BaseGain = atof(initest.value);
                else if(!strcmp(initest.key,"bootsong"))
                    Game->BootSong = atoi(initest.value);
                else if(!strcmp(initest.key,"portafix"))
                    Game->PortaFix = atoi(initest.value);
                else if(!strcmp(initest.key,"audiodevice"))
                    strcpy(Game->AudioDevice,initest.value);
                else if(!strcmp(initest.key,"audiobuffer"))
                    Game->AudioBuffer = atoi(initest.value);
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
            Game->AutoPlay = (int)strtol(argv[i],NULL,0);
        }
        else if((!strcmp(argv[i],"-ini") || !strcmp(argv[i],"--ini-path")) && i<argc)
        {
            i++;
            strcpy(QP_IniPath,argv[i]);
        }
        else if(!strcmp(argv[i],"-w") || !strcmp(argv[i],"--wavlog"))
        {
            Game->WavLog=1;
        }
        else if(!strcmp(argv[i],"-v") || !strcmp(argv[i],"--vgmlog"))
        {
            Game->VgmLog=1;
        }
        else
        {
            if(standard_args == 0)
                strcpy(Game->Name, argv[i]);
            else if(standard_args == 1)
                Game->AutoPlay = (int)strtol(argv[i],NULL,0);
            else
                break;
            standard_args++;
        }

    }

    Game->QDrv = QDrv;

    if(LoadGame(Game))
    {
        SDL_Quit();
        return -1;
    }

    InitGame(Game);

    ui_init();
    ui_main();
    ui_deinit();

    QPAudio_Close(Audio);

    // Audio must be closed or locked before calling this
    DeInitGame(Game);

    SDL_Quit();
    UnloadGame(Game);
    free(QDrv);

    return 0;
}
