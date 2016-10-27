/*
    Generic helper functions to help with file I/O
*/
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "fileio.h"
#include "string.h"
#include "errno.h"

    char fileio_error[100];

int load_file(char* filename, uint8_t** dataptr, uint32_t* filesize)
{
    FILE* sourcefile;
    sourcefile = fopen(filename,"rb");

    if(!sourcefile)
    {
        strcpy(fileio_error,strerror(errno));
        fprintf(stderr,"Could not open %s\n",filename);
        perror("Error");
        return -1;
    }
    fseek(sourcefile,0,SEEK_END);
    *filesize = ftell(sourcefile);
    rewind(sourcefile);
    *dataptr = (uint8_t*)malloc((*filesize)+128);

    int32_t res = fread(*dataptr,1,*filesize,sourcefile);
    if(res != *filesize)
    {
        strcpy(fileio_error,"Read error");
        fputs(fileio_error,stderr);
        fclose(sourcefile);
        return -1;
    }
    fclose(sourcefile);

    return 0;
}

// This does not allocate new resources
int read_file(char* filename, uint8_t* dataptr, uint32_t load_size, uint32_t load_offset, int byteswap, uint32_t* fsize)
{
    uint32_t cnt;
    uint32_t filesize;
    uint8_t temp;

    FILE* sourcefile;
    sourcefile = fopen(filename,"rb");

    if(!sourcefile)
    {
        strcpy(fileio_error,strerror(errno));
        fprintf(stderr,"Could not open %s\n",filename);
        perror("Error");
        return -1;
    }
    fseek(sourcefile,0,SEEK_END);
    filesize = ftell(sourcefile);

    // filesize limit (for sanity checks)
    if(fsize && *fsize != 0 && filesize > *fsize)
        filesize = *fsize;

    if(load_offset >= filesize)
    {
        //strcpy(fileio_error,"Read offset exceeds file size  );
        sprintf(fileio_error,"Read offset (%d) exceeds file size (%d)\n",load_offset,filesize);
        fputs(fileio_error,stderr);
        fclose(sourcefile);
        return -1;
    }

    if(load_size == 0)
        load_size = filesize;

    if(load_size+load_offset > filesize)
    {
        sprintf(fileio_error,"Warning: Read length (%d) exceeds file size (%d)\n",load_size+load_offset,filesize);
        fputs(fileio_error,stderr);
        load_size = filesize - load_offset;
    }

    rewind(sourcefile);

    if(load_offset)
        fseek(sourcefile,load_offset,SEEK_SET);

    int32_t res = fread(dataptr,1,load_size,sourcefile);
    if(res != load_size)
    {
        strcpy(fileio_error,"Read error");
        fputs(fileio_error,stderr);
        fclose(sourcefile);
        return -1;
    }

    if(byteswap)
    {
        for(cnt=0;cnt<load_size;cnt+=2)
        {
            temp = dataptr[cnt];
            dataptr[cnt] = dataptr[cnt+1];
            dataptr[cnt+1] = temp;
        }
    }

    if(fsize)
        *fsize = load_size;

    fclose(sourcefile);

    return 0;
}


int write_file(char* filename, uint8_t* dataptr, uint32_t datasize)
{
    FILE *destfile;

    destfile = fopen(filename,"wb");
    if(!destfile)
    {
        fprintf(stderr,"Could not open %s\n",filename);
        return -1;
    }
    int32_t res = fwrite(dataptr, 1, datasize, destfile);
    if(res != datasize)
    {
        strcpy(fileio_error,"Read error");
        fputs(fileio_error,stderr);
        fprintf(stderr,"Writing error\n");
        fclose(destfile);
        return -1;
    }
    fclose(destfile);
    printf("%d bytes written to %s.\n",res,filename);
    return 0;
}

char* my_strerror(char* filename)
{
    static char msg[100];
    snprintf(msg,100,"\n'%s': %s",filename,fileio_error);
    return msg;
}
