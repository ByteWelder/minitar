#include <stdlib.h>
#include "minitar.h"

struct minitar* minitar_open(const char* path)
{
    FILE* fp = fopen(path, "r");
    if(!fp) return NULL;
    struct minitar* mp = malloc(sizeof(struct minitar));
    if(!mp) { fclose(fp); return NULL; }
    mp->stream = fp;
    return mp;
}

int minitar_close(struct minitar* mp)
{
    int rc = fclose(mp->stream);
    free(mp);
    if(rc) return rc;
    return 0;
}