/*
    global macros
*/
#ifndef MACRO_H_INCLUDED
#define MACRO_H_INCLUDED

#ifdef DEBUG
#include <stdio.h>
#define Q_DEBUG(...) printf(__VA_ARGS__)
#else
#define Q_DEBUG(...)
#endif

#endif // MACRO_H_INCLUDED
