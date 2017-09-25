/*
    Ini file handling
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "ini.h"

#define VALID_KEY(arg) isalnum(arg) || arg == '_' || arg == '-'

const char* ini_error[INI_MAX_STATUS] =
{
    "Ok","Reached eof","File load error","Unexpected eof","Unterminated string"
};

int ini_open(char* filename, inifile_t* ini)
{
    FILE* sourcefile;
    sourcefile = fopen(filename,"r");

    if(!sourcefile)
    {
        // just run strerror(errno) after this
        ini->f = NULL;
        ini->status = INI_FILE_LOAD_ERROR;
        return -1;
    }

    ini->f = sourcefile;

    // null strings
    ini->key[0] = 0;
    ini->section[0] = 0;
    ini->value[0] = 0;
    ini->status=0;

    return 0;
}

int ini_newline(inifile_t* ini)
{
    if(ini->c == '\r')
        ini->c = getc(ini->f);
    if(ini->c == '\n')
        return 1;
    return 0;
}

int ini_readkey(inifile_t* ini)
{
    int count=0;
    while(VALID_KEY(ini->c))
    {
        ini->key[count++] = tolower(ini->c);

        ini->c = getc(ini->f);

        if(ini->c == EOF)
            return -1;
        if(ini_newline(ini))
            return -1;
    }

    ini->key[count] = 0;
    //printf("Key: %s\n",ini->key);
    return 0;
}

int ini_readspaces(inifile_t* ini)
{
    while(1)
    {
        if(ini->c == EOF)
            return -1;
        if(ini_newline(ini))
            return -1;
        if(!isspace(ini->c))
            return 0;
        ini->c = getc(ini->f);
    }
}

void ini_escape(inifile_t* ini)
{
    ini->c = getc(ini->f);
    switch(ini->c)
    {
    case 'n':
        ini->c = '\n';break;
    case 'r':
        ini->c = '\r';break;
    case 't':
        ini->c = '\t';break;
    default: break;
    }
}

int ini_readvalue(inifile_t* ini)
{
    int count=0;

    // enclosed string
    if(ini->c == '"')
    {
        ini->c = getc(ini->f);

        while(ini->c != '"')
        {
            if(ini->c == '\\')
                ini_escape(ini);

            if(ini->c == EOF || ini_newline(ini)) // unterminated
            {
                ini->status = INI_UNTERMINATED_STRING;
                return -1;
            }

            ini->value[count++] = ini->c;

            ini->c = getc(ini->f);
        }

        while(ini->c != EOF && !ini_newline(ini))
            ini->c = getc(ini->f);

    }
    else
    {
        while(!ini_newline(ini))
        {
            if(ini->c == '\\')
                ini_escape(ini);

            ini->value[count++] = ini->c;

            ini->c = getc(ini->f);

            if(ini->c == EOF)
                break;
        }
    }

    ini->value[count] = 0;
    //printf("Value: %s\n",ini->value);
    return 0;
}

int ini_readsection(inifile_t* ini)
{
    int count=0;

    while(ini->c != ']')
    {
        ini->section[count++] = tolower(ini->c);

        ini->c = getc(ini->f);

        if(ini->c == EOF)
        {
            ini->status = INI_UNEXPECTED_EOF;
            return -1;
        }
    }

    ini->section[count] = 0;
    return 0;
}


int ini_skipline(inifile_t* ini)
{
    while(!ini_newline(ini))
    {
        if(ini->c == EOF)
            return -1;
        ini->c = getc(ini->f);
    }
    return 0;
}

int ini_readnext(inifile_t* ini)
{
    int skip=1;
    while(skip)
    {
        ini->c = getc(ini->f);
        if(ini->c == '[')
        {
            ini->c = getc(ini->f);
            if(ini_readsection(ini))
                return -1;
            if(ini_skipline(ini))
                return -1;
        }
        else if(VALID_KEY(ini->c))
        {
            if(ini_readkey(ini) || ini_readspaces(ini) || (ini->c != '='))
                return -1;
            ini->c = getc(ini->f);
            if(ini_newline(ini) || ini_readspaces(ini) || ini_readvalue(ini))
                return -1;
            return 0;
        }
        else if(ini_skipline(ini))
            return -1;
    }
    return -1;
}

int ini_close(inifile_t* ini)
{
    if(ini->f)
        fclose(ini->f);
    return 0;
}
