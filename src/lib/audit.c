/*
 this audit is not quite perfect yet, does not check that the ini files
 contain all required values...
*/

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "../qp.h"
#include "ini.h"
#include "audit.h"

int AuditRoms(void* data)
{
    QP_Audit* audit = data;
    audit->AuditFlag = 1;

    FILE* file;
    struct QP_AuditRom* rom;

    audit->OkCount = audit->BadCount = audit->CheckCount = 0;
    int okflag;
    int i,j;
    for(i=0;i<audit->Count;i++)
    {
        audit->Entry[i].RomOk = 0;
        okflag = 1;
        for(j=0;j<audit->Entry[i].RomCount;j++)
        {
            rom = &audit->Entry[i].Rom[j];
            file = NULL;
            file = fopen(rom->Path,"rb");
            if(file)
            {
                rom->Ok = 1;
                fclose(file);
            }
            else
            {
                rom->Ok = 0;
                okflag = 0;
            }
        }
        if(okflag)
        {
            audit->OkCount++;
            audit->Entry[i].RomOk = 1;
        }
        else
        {
            audit->BadCount++;
        }
        audit->CheckCount++;
    }
    audit->AuditFlag = 0;
    return 0;
}

void WriteRomEntry(struct QP_AuditRom *rom,int PathType,char* Path1,char* Path2)
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

void WriteAuditEntry(QP_AuditEntry *entry, char *name)
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
            entry->RomOk = 0;
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
    QP_AuditEntry *ea = (QP_AuditEntry*)a;
    QP_AuditEntry *eb = (QP_AuditEntry*)b;
    return strcmp(ea->Name,eb->Name);
}

int AuditGames(void* data)
{
    QP_Audit* audit = data;

    char dir[128], temp[256];
    audit->AuditFlag = 2;
    audit->Count = audit->CheckCount = audit-> OkCount = audit->BadCount = 0;
    memset(audit->Entry,0,sizeof(audit->Entry));

    snprintf(dir,127,"./%s/",QP_IniPath);

    int i=0;

    DIR* dp;
    struct dirent *ep;
    //QP_AuditEntry *ae;

    dp = opendir(dir);
    if(dp != NULL)
    {
        while((ep=readdir(dp)) != NULL)
        {
            //ae = (QP_AuditEntry*)malloc(sizeof(QP_AuditEntry));
            if(sscanf(ep->d_name,"%[^.].ini",temp))
            {
                //ae = (QP_AuditEntry*)malloc(sizeof(QP_AuditEntry));
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

    if(!audit->Count)
        audit->Count = -1;
    else
        qsort(audit->Entry,audit->Count,sizeof(QP_AuditEntry),AuditSortCompare);

    audit->AuditFlag = 0;
    return 0;
}

