/*
    Generic helper functions to help with file I/O
*/
#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

    char fileio_error[100];

int load_file(char* filename, uint8_t** dataptr, uint32_t* filesize);
int read_file(char* filename, uint8_t* dataptr, uint32_t load_size, uint32_t load_offset, int byteswap, uint32_t* fsize);
int write_file(char* filename, uint8_t* dataptr, uint32_t datasize);

char* my_strerror(char* filename);

#endif // FILEIO_H_INCLUDED
