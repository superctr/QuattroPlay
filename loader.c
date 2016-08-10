/*
    File loader and driver initialization
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "SDL2/SDL.h"
#include "drv/quattro.h"
#include "c352.h"
#include "ui.h"
#include "ini.h"
#include "loader.h"
#include "fileio.h"

// Loads game ini, then the sound data and wave roms...
int LoadGame(Q_State *Q, char *gamename)
{
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
    int chipfreq = 0;

    int patchtype_set = 0;
    int patchaddr_set = 0;
    int patchdata_set = 0;
    int patchcount = 0;

    int pitchoverflow = -1;

    int patchtype[64];
    int patchaddr[64];
    int patchdata[64];

    static char wave_filename[16][128];

    static char data_filename[128];

    msgstring[0] = 0;
    mcutype[0] = 0;
    path[0] = 0;

    sprintf(msgstring,"Failed to load '%s':",gamename);
    int loadok = strlen(msgstring);

    snprintf(filename,127,"%s/%s.ini",L_IniPath,gamename);

#ifdef DEBUG
    printf("Now loading '%s' ...\n",filename);
#endif

    memset(wave_pos,0,sizeof(wave_pos));
    memset(wave_length,0,sizeof(wave_length));
    memset(wave_offset,0,sizeof(wave_offset));
    memset(wave_filename,0,sizeof(wave_filename));

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
                    strcpy(L_GameTitle,initest.value);
                else if(!strcmp(initest.key,"path"))
                    strcpy(path,initest.value);
                else if(!strcmp(initest.key,"filename"))
                    strcpy(data_filename,initest.value);
                else if(!strcmp(initest.key,"type"))
                    strcpy(mcutype,initest.value);
                else if(!strcmp(initest.key,"byteswap"))
                    byteswap = atoi(initest.value) & 1;
                else if(!strcmp(initest.key,"gain"))
                    gamegain = atof(initest.value);
                else if(!strcmp(initest.key,"muterear"))
                    muterear = atoi(initest.value);
                else if(!strcmp(initest.key,"chipfreq"))
                    chipfreq = atoi(initest.value);
                else if(!strcmp(initest.key,"pitchtab"))
                    pitchoverflow = strtol(initest.value,NULL,0);
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
                Q_DEBUG("playlist %s = %s\n",initest.key,initest.value);



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
        strcpy(path,gamename);

    L_WaveMask=0;
    L_DataSize = 0x80000;
    L_Data = (uint8_t*)malloc(L_DataSize);

#ifdef DEBUG
    printf("Game title: '%s'\n",L_GameTitle);
    printf("Data filename: '%s'\n",data_filename);
    printf("Wave count: %d\n",wave_count+1);
    if(byteswap)
        printf("Data file byteswapped\n");
#endif
    snprintf(filename,127,"%s/%s/%s",L_DataPath,path,data_filename);
    if(read_file(filename,L_Data,0,0,byteswap,&L_DataSize))
    {
        strcat(msgstring,my_strerror(filename));
        //return -1;
    }

    int i;
    for(i=0;i<patchcount;i++)
    {
        printf("Patch type %d addr %06x data %06x\n",patchtype[i],patchaddr[i],patchdata[i]);

        if(patchaddr[i]+patchtype[i] > L_DataSize)
            printf("patch address out of bounds\n");
        else if(patchtype[i] == 1) // byte
            *(uint8_t*)(L_Data+patchaddr[i]) = patchdata[i];
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

            *(uint16_t*)(L_Data+patchaddr_set) = patchdata_set&0xffff;
            *(uint8_t*)(L_Data+patchaddr_set+2) = patchdata_set>>16;
        }
        else //if (patchtype[i] == 2) // word
            *(uint16_t*)(L_Data+patchaddr[i]) = patchdata[i];
    }

    L_WaveData = (uint8_t*)malloc(0x1000000);
    memset(L_WaveData,0,0x1000000);
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
        snprintf(filename,127,"%s/%s/%s",L_WavePath,path,wave_filename[i]);
        if(read_file(filename,L_WaveData+wave_pos[i],wave_length[i],wave_offset[i],0,&wave_maxlen))
            strcat(msgstring,my_strerror(filename));
        else
            L_WaveMask |= wave_pos[i]+wave_length[i]-1;
    }

#ifdef DEBUG
    printf("Wave Mask = %06x\n",L_WaveMask);
#endif

    if(loadok != strlen(msgstring))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error",msgstring,NULL);
        return -1;
    }

    // Setup sound driver.
    memset(&Q->Chip,0,sizeof(Q->Chip));

    C352_init(&Q->Chip,chipfreq);
    Q->Chip.vgm_log = 0;

    Q->EnablePitchOverflow = pitchoverflow >= 0 ? 1 : 0;
    Q->PitchOverflow = pitchoverflow&0xffff;

    Q->Chip.wave = L_WaveData;
    Q->Chip.wave_mask = L_WaveMask;
    Q->McuData = L_Data;

    return 0;
}

int UnloadGame(Q_State* Q)
{
    free(L_Data);
    free(L_WaveData);

    //free(Q_Chip);

    return 0;
}
