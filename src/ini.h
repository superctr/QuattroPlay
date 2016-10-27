#ifndef INI_H_INCLUDED
#define INI_H_INCLUDED
#include "stdint.h"

enum {
    INI_OK = 0,
    INI_EOF = 1,
    INI_FILE_LOAD_ERROR = 2,
    INI_UNEXPECTED_EOF = 3,
    INI_UNTERMINATED_STRING = 4,
    INI_MAX_STATUS = 5,
};

typedef struct {

    FILE * f;

    char section[64];
    char key[128];
    char value[128];

    int c;
    int status;
} inifile_t;

const char* ini_error[INI_MAX_STATUS];

int ini_open(char* filename, inifile_t* ini);
int ini_readnext();
int ini_close(inifile_t* ini);


#endif // INI_H_INCLUDED
