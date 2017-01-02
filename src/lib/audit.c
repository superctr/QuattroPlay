/*
 this audit is not quite perfect yet, does not check that the ini files
 contain all required values...
*/

#include "stdio.h"
#include "sys/types.h"
#include "dirent.h"
#include "stdlib.h"
#include "string.h"

#include "../qp.h"
#include "ini.h"
#include "audit.h"


void WriteRomEntry(struct QPAuditRom *rom,int PathType,char* Path1,char* Path2)
{
    switch(PathType)
    {
    default:
    case AUDIT_PATH_DATA:
        snprintf(rom->Path,127,"%s/%s/%s",QP_DataPath,Path1,Path2);
        break;
    case AUDIT_PATH_WAVE:
        snprintf(rom->Path,127,"%s/%s/%s",QP_WavePath,Path1,Path2);
        break;
    }
    /*
    FILE* file;
    file = NULL;
    file = fopen(rom->Path,"rb");

    // could do CRC and maybe size check here as well in the future
    if(file)
    {
        rom->Ok=1;
        fclose(file);
        printf("%s OK, ",Path2);
    }
    else
    {
        rom->Ok=0;
        printf("%s NG, ",Path2);
    }
    */
}

void WriteAuditEntry(QPAuditEntry *entry, char *name)
{
    char filename[128];
    char rompath[128];
    int blah;
    snprintf(filename,127,"%s/%s.ini",QP_IniPath,name);
    strcpy(entry->Name,name);
    strcpy(rompath,name);
    strcpy(entry->DisplayName,"");
    entry->HasMeta=0;
    entry->HasPlaylist=0;
    entry->RomCount=0;
    inifile_t initest;
    if(!ini_open(filename,&initest))
    {
        entry->IniOk = 1;

        while(!ini_readnext(&initest))
        {
            if(!strcmp(initest.section,"data"))
            {
                if(!strcmp(initest.key,"name"))
                    strcpy(entry->DisplayName,initest.value);
                else if(!strcmp(initest.key,"path"))
                    strcpy(rompath,initest.value);
                else if(!strcmp(initest.key,"filename"))
                {
                    if(entry->RomCount < AUDIT_MAX_ROMS)
                        WriteRomEntry(&entry->Rom[entry->RomCount++],AUDIT_PATH_DATA,rompath,initest.value);
                }
            }
            else if(sscanf(initest.section,"wave.%d",&blah) && !strcmp(initest.key,"filename"))
            {
                if(entry->RomCount < AUDIT_MAX_ROMS)
                    WriteRomEntry(&entry->Rom[entry->RomCount++],AUDIT_PATH_WAVE,rompath,initest.value);
            }
            else if(!strcmp(initest.section,"playlist"))
            {
                if(!entry->HasPlaylist)
                    entry->HasPlaylist = 1;
                if(!strcmp(initest.key,"wip"))
                    entry->HasPlaylist = 2;
            }
        }
    }
    else
    {
        entry->IniOk = 0;
    }
    ini_close(&initest);
}

int AuditSortCompare(const void *a, const void *b)
{
    QPAuditEntry *ea = (QPAuditEntry*)a;
    QPAuditEntry *eb = (QPAuditEntry*)b;
    return strcmp(ea->Name,eb->Name);
}

int AuditGames(QPAudit* audit)
{
    char dir[128], temp[128];

    snprintf(dir,127,"./%s/",QP_IniPath);

    int i=0;

    DIR* dp;
    struct dirent *ep;
    //QPAuditEntry *ae;

    dp = opendir(dir);
    if(dp != NULL)
    {
        while((ep=readdir(dp)) != NULL)
        {
            //ae = (QPAuditEntry*)malloc(sizeof(QPAuditEntry));
            if(sscanf(ep->d_name,"%[^.].ini",temp))
            {
                //ae = (QPAuditEntry*)malloc(sizeof(QPAuditEntry));
                WriteAuditEntry(&audit->Entry[i],temp);
                //audit->Entry[i] = ae;
                if(audit->Entry[i].IniOk)
                {
                    ++audit->Count;
                    if(++i == AUDIT_MAX_COUNT)
                        break;
                }
            }
        }
        (void)closedir(dp);
    }
    else
        return -1;

    qsort(audit->Entry,audit->Count,sizeof(QPAuditEntry),AuditSortCompare);

    return 0;
}

