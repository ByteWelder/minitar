#include <stdlib.h>
#include "minitar.h"
#include "tar.h"

int minitar_read_header(struct minitar* mp, struct tar_header* hdr);
int minitar_validate_header(struct tar_header* hdr);
void minitar_parse_tar_header(struct tar_header* hdr, struct minitar_entry_metadata* metadata);
struct minitar_entry* minitar_dup_entry(struct minitar_entry* original);
char* minitar_read_file(struct minitar_entry_metadata* metadata, struct minitar* mp);

struct minitar* minitar_open(const char* path)
{
    FILE* fp = fopen(path, "rb"); // On some systems, this might be necessary to read the file properly.
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

static struct minitar_entry* minitar_attempt_read_entry(struct minitar* mp, int* valid)
{
    struct minitar_entry entry;
    struct tar_header hdr;
    if(!minitar_read_header(mp, &hdr))
    {
        *valid = 1; // we are at end-of-file
        return NULL;
    }
    if(!minitar_validate_header(&hdr))
    {
        *valid = 0;
        return NULL;
    }
    *valid = 1;
    minitar_parse_tar_header(&hdr, &entry.metadata);
    char* buf = minitar_read_file(&entry.metadata, mp);
    if(!buf) return NULL;
    entry.ptr = buf;
    return minitar_dup_entry(&entry);
}

struct minitar_entry* minitar_read_entry(struct minitar* mp)
{
    int valid;
    struct minitar_entry* result;
    do {
        result = minitar_attempt_read_entry(mp, &valid);
    } while(!valid);
    return result;
}

void minitar_free_entry(struct minitar_entry* entry)
{
    free(entry->ptr);
    free(entry);
}