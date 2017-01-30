#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "SDL2/SDL.h"

#include "qp.h"

#include "lib/vgm.h"
#include "lib/audit.h"
#include "lib/ini.h"

#include "ui/ui.h"

int main(int argc, char *argv[])
{
    int loop = 0;
    int val;
    SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER);

    Audio = (QP_Audio*)malloc(sizeof(QP_Audio));
    memset(Audio,0,sizeof(QP_Audio));

    Game = (QP_Game*)malloc(sizeof(QP_Game));
    memset(Game,0,sizeof(QP_Game));

    Audit = (QP_Audit*)malloc(sizeof(QP_Audit));
    memset(Audit,0,sizeof(QP_Audit));

    DriverInterface=0;

    if(!Audio || !Game)
        return -1;

    Game->AutoPlay = -1;

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

    //Game->QDrv = QDrv;

    if(!strlen(Game->Name))
    {
        loop=1;

        AuditGames(Audit);
    }

    if(ui_init())
    {
        SDL_Quit();
        return -1;
    }

    while(1)
    {
        if(loop)
        {
            val = ui_main(SCR_SELECT);
            if(!val)
                break;
            --val;
            strcpy(Game->Name,Audit->Entry[val].Name);
        }

        val = (LoadGame(Game) || InitGame(Game));
        if(!val)
        {
            QP_AudioSetPause(Audio,0);
            Audio->state.UpdateRequest = QPAUDIO_CHIP_PLAY|QPAUDIO_DRV_PLAY;

            ui_main(loop ? SCR_PLAYLIST : SCR_MAIN);

            QP_AudioClose(Audio);

            // Audio must be closed or locked before calling this
            DeInitGame(Game);
        }
        UnloadGame(Game);

        if(!loop)
            break;
    }

    ui_deinit();
    SDL_Quit();

    free(Audit);
    free(Audio);
    free(Game);

    return 0;
}
