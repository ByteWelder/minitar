#ifndef MINITAR_H
#define MINITAR_H
#include <stdio.h>

struct minitar
{
    FILE* stream;
};

struct minitar* minitar_open(const char* pathname);
int minitar_close(struct minitar* mp);

#endif