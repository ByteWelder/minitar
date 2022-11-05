#include <stdlib.h>
#include "minitar.h"
#include "tar.h"

int minitar_read_header(struct minitar* mp, struct tar_header* hdr);
int minitar_validate_header(struct tar_header* hdr);
void minitar_parse_tar_header(struct tar_header* hdr, struct minitar_entry_metadata* metadata);
struct minitar_entry* minitar_dup_entry(struct minitar_entry* original);

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
    // FIXME: Actually read the file and place it in buf.
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
    free(entry); // FIXME: Also free the file's content, when it's placed in buf.
}