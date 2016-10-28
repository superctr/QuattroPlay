/*
    File loader and driver initialization
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "SDL2/SDL.h"
#include "qp.h"
// #include "ui.h"
#include "vgm.h"
#include "ini.h"
#include "fileio.h"

// Loads game ini, then the sound data and wave roms...
int LoadGame(game_t *G)
{
    Q_State* Q = G->QDrv;

    static char msgstring[2048];

    static char filename[128];
    static char path[128];
    //static char gamehackname[128];
    static char wave0[16];
    static char wave1[16];
    static char mcutype[16];

    int byteswap = 0;
    int wave_count = 0;
    int wave_pos[16];
    int wave_length[16];
    int wave_offset[16];
    unsigned int wave_maxlen; // max length of wave roms.
    G->ChipFreq = 0;

    int patchtype_set = 0;
    int patchaddr_set = 0;
    int patchdata_set = 0;
    int patchcount = 0;

    unsigned int action_id = 0;
    unsigned int action_reg = 0;
    unsigned int action_data = 0;

    int patchtype[64];
    int patchaddr[64];
    int patchdata[64];

    static char wave_filename[16][128];

    static char data_filename[128];

    msgstring[0] = 0;
    mcutype[0] = 0;
    path[0] = 0;

    sprintf(msgstring,"Failed to load '%s':",G->Name);
    int loadok = strlen(msgstring);

    snprintf(filename,127,"%s/%s.ini",QP_IniPath,G->Name);

#ifdef DEBUG
    printf("Now loading '%s' ...\n",filename);
#endif

    strcpy(G->Title,G->Name);
    memset(wave_pos,0,sizeof(wave_pos));
    memset(wave_length,0,sizeof(wave_length));
    memset(wave_offset,0,sizeof(wave_offset));
    memset(wave_filename,0,sizeof(wave_filename));
    memset(G->Action,0,sizeof(G->Action));

    inifile_t initest;
    if(!ini_open(filename,&initest))
    {
        while(!ini_readnext(&initest))
        {
            //printf("'%s'.'%s' = '%s'\n",initest.section,initest.key,initest.value);
            snprintf(wave0,15,"wave.%d",wave_count);
            snprintf(wave1,15,"wave.%d",wave_count+1);

            // this will be updated with more options as needed.
            if(!strcmp(initest.section,"data"))
            {
                if(!strcmp(initest.key,"name"))
                    strcpy(G->Title,initest.value);
                else if(!strcmp(initest.key,"path"))
                    strcpy(path,initest.value);
                else if(!strcmp(initest.key,"filename"))
                    strcpy(data_filename,initest.value);
                else if(!strcmp(initest.key,"type"))
                    strcpy(mcutype,initest.value);
                else if(!strcmp(initest.key,"byteswap"))
                    byteswap = atoi(initest.value) & 1;
                else if(!strcmp(initest.key,"gain"))
                    G->Gain = atof(initest.value);
                else if(!strcmp(initest.key,"muterear"))
                    G->MuteRear = atoi(initest.value);
                else if(!strcmp(initest.key,"chipfreq"))
                    G->ChipFreq = atoi(initest.value);
//                else if(!strcmp(initest.key,"gamehack"))
//                    strcpy(gamehackname,initest.value);
            }

            if(!strcmp(initest.section,"patch"))
            {
                patchtype_set=0;
                patchdata_set = strtol(initest.value,NULL,0);

                if(!strcmp(initest.key,"address"))
                    patchaddr_set = patchdata_set;
                else if(!strcmp(initest.key,"song"))
                    patchaddr_set = patchdata_set*3;
                else if(!strcmp(initest.key,"byte"))
                    patchtype_set = 1;
                else if(!strcmp(initest.key,"word"))
                    patchtype_set = 2;
                else if(!strcmp(initest.key,"pos"))
                    patchtype_set = 3;

                if(patchtype_set)
                {
                    patchtype[patchcount] = patchtype_set;
                    patchaddr[patchcount] = patchaddr_set;
                    patchdata[patchcount] = patchdata_set;
                    patchaddr_set+=patchtype_set;
                    patchcount++;
                }
            }

            // at the next wave rom section?
            if(!strcmp(initest.section,wave1))
            {
                if(wave_count < 16)
                    wave_count++;

                snprintf(wave0,15,"wave.%d",wave_count);
            }
            if(!strcmp(initest.section,wave0))
            {
                if(!strcmp(initest.key,"filename"))
                    strcpy(wave_filename[wave_count],initest.value);
                else if(!strcmp(initest.key,"length"))
                    wave_length[wave_count] = strtol(initest.value,NULL,0);
                else if(!strcmp(initest.key,"position"))
                    wave_pos[wave_count] = strtol(initest.value,NULL,0);
                else if(!strcmp(initest.key,"offset"))
                    wave_offset[wave_count] = strtol(initest.value,NULL,0);
            }
            if(!strcmp(initest.section,"playlist"))
            {
                // nothing here... yet.
                Q_DEBUG("playlist %s = %s\n",initest.key,initest.value);
            }
            if(sscanf(initest.section,"action.%d",&action_id)==1 && action_id < 10)
            {
                if(sscanf(initest.key,"r%x",&action_reg)==1)
                {
                    action_data = strtol(initest.value,NULL,0);
                    G->Action[action_id].reg[G->Action[action_id].cnt] = action_reg;
                    G->Action[action_id].data[G->Action[action_id].cnt] = action_data;
                    Q_DEBUG("action %d (%02x) =  r%02x = %04x\n",action_id,G->Action[action_id].cnt,action_reg,action_data);
                    G->Action[action_id].cnt++;
                }
            }
        }
    }

    if(initest.status)
    {
        if(initest.status == INI_FILE_LOAD_ERROR)
            strcat(msgstring,my_strerror(filename));
        else
            strcat(msgstring,ini_error[initest.status]);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",msgstring,NULL);
        ini_close(&initest);
        return -1;
    }

    ini_close(&initest);

    if(strlen(path) == 0)
        strcpy(path,G->Name);

    G->WaveMask=0;
    G->DataSize = 0x80000;
    G->Data = (uint8_t*)malloc(G->DataSize);

#ifdef DEBUG
    printf("Game title: '%s'\n",G->Title);
    printf("Data filename: '%s'\n",data_filename);
    printf("Wave count: %d\n",wave_count+1);
    if(byteswap)
        printf("Data file byteswapped\n");
#endif
    snprintf(filename,127,"%s/%s/%s",QP_DataPath,path,data_filename);
    if(read_file(filename,G->Data,0,0,byteswap,&G->DataSize))
    {
        strcat(msgstring,my_strerror(filename));
        //return -1;
    }

    int i;
    for(i=0;i<patchcount;i++)
    {
        printf("Patch type %d addr %06x data %06x\n",patchtype[i],patchaddr[i],patchdata[i]);

        if(patchaddr[i]+patchtype[i] > G->DataSize)
            printf("patch address out of bounds\n");
        else if(patchtype[i] == 1) // byte
            *(uint8_t*)(G->Data+patchaddr[i]) = patchdata[i];
        else if(patchtype[i] == 3) // song table
        {
            patchaddr_set = patchaddr[i];
            if(!strcmp(mcutype,"H8")) // for most cases....
            {
                if(patchaddr_set < 0x1800)
                    patchaddr_set += 0x800e;
                patchdata_set = patchdata[i];
            }
            else
            {
                if(patchaddr_set < 0x1800)
                    patchaddr_set += 0x1000e;
                patchdata_set = patchdata[i] + 0x200000;
            }

            *(uint16_t*)(G->Data+patchaddr_set) = patchdata_set&0xffff;
            *(uint8_t*)(G->Data+patchaddr_set+2) = patchdata_set>>16;
        }
        else //if (patchtype[i] == 2) // word
            *(uint16_t*)(G->Data+patchaddr[i]) = patchdata[i];
    }

    G->WaveData = (uint8_t*)malloc(0x1000000);
    memset(G->WaveData,0,0x1000000);
    for(i=0;i<wave_count+1;i++)
    {
#ifdef DEBUG
        printf("Wave %d\n",i);
        printf("\tFilename: '%s'\n",wave_filename[i]);
        printf("\tPosition: %06x\n",wave_pos[i]);
        printf("\tLength: %06x\n",wave_length[i]);
        printf("\tOffset: %06x\n",wave_offset[i]);
#endif
        wave_maxlen = 0x1000000 - wave_pos[i];
        snprintf(filename,127,"%s/%s/%s",QP_WavePath,path,wave_filename[i]);
        if(read_file(filename,G->WaveData+wave_pos[i],wave_length[i],wave_offset[i],0,&wave_maxlen))
            strcat(msgstring,my_strerror(filename));
        else
            G->WaveMask |= wave_pos[i]+wave_length[i]-1;
    }

#ifdef DEBUG
    printf("Wave Mask = %06x\n",G->WaveMask);
#endif

    if(loadok != strlen(msgstring))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",msgstring,NULL);
        return -1;
    }

    G->QDrv = Q;
    return 0;
}

int UnloadGame(game_t *G)
{
    free(G->Data);
    free(G->WaveData);
    //free(Q_Chip);
    return 0;
}

void InitGame(game_t *Game)
{

    // Initialize sound chip and some initial sound driver parameters.
    memset(&QDrv->Chip,0,sizeof(QDrv->Chip));

    QDrv->ChipClock = Game->ChipFreq;
    C352_init(&QDrv->Chip,Game->ChipFreq);
    QDrv->Chip.vgm_log = 0;

    QDrv->Chip.wave = Game->WaveData;
    QDrv->Chip.wave_mask = Game->WaveMask;
    QDrv->McuData = Game->Data;

    static char filename[FILENAME_MAX];

    char* audiodev = NULL;
    if(strlen(Game->AudioDevice))
        strcpy(audiodev,Game->AudioDevice);

    if(Game->VgmLog)
    {
        strcpy(filename,"qp_log.vgm");
        if(Game->AutoPlay >= 0)
        {
            sprintf(filename,"%s_%03x.vgm",Game->Name,Game->AutoPlay&0x7ff);
        }
        vgm_open(filename);
        vgm_datablock(0x92,0x1000000,QDrv->Chip.wave,0x1000000,QDrv->Chip.wave_mask,0);
        QDrv->Chip.vgm_log = 1;
    }

    QDrv->PortaFix=Game->PortaFix;
    QDrv->BootSong=Game->BootSong;
    Q_Init(QDrv);

    QPAudio_Init(Audio,QDrv,QDrv->Chip.rate,Game->AudioBuffer,audiodev);

    if(Game->AutoPlay >= 0)
        QDrv->BootSong=2;

    Audio->state.AutoPlaySong = Game->AutoPlay;

    if(Game->WavLog)
    {
        strcpy(filename,"qp_log.wav");
        if(Game->AutoPlay >= 0)
        {
            sprintf(filename,"%s_%03x.wav",Game->Name,Game->AutoPlay&0x7ff);
        }
        QPAudio_WavOpen(Audio,filename);
    }
}

void DeInitGame(game_t *Game)
{
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
        vgm_write_tag(strlen(Game->Title) ? Game->Title : Game->Name,Game->AutoPlay);
        vgm_close();
        SDL_UnlockAudioDevice(Audio->dev);
    }

    Q_Deinit(QDrv);
}

// Perform register action (song triggers).
void GameDoAction(game_t *G,unsigned int id)
{
    if(id > 9)
        return;
    int i;
    for(i=0;i<G->Action[id].cnt;i++)
    {
        G->QDrv->Register[G->Action[id].reg[i]&0xff] = G->Action[id].data[i];
    }
    return;
}

