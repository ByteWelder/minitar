#define _IN_MINITAR
#include "tar.h"
#include "minitar.h"
#include <stdlib.h>
#include <string.h>

// all of these are defined in util.c
int minitar_read_header(struct minitar*, struct tar_header*);
int minitar_validate_header(struct tar_header*);
void minitar_parse_tar_header(struct tar_header*, struct minitar_entry_metadata*);
struct minitar_entry* minitar_dup_entry(struct minitar_entry*);
char* minitar_read_file_contents(struct minitar_entry_metadata*, struct minitar*);

struct minitar* minitar_open(const char* pathname)
{
    FILE* fp = fopen(pathname, "rb"); // On some systems, this might be necessary to read the file properly.
    if (!fp) return NULL;
    struct minitar* mp = malloc(sizeof(struct minitar));
    if (!mp)
    {
        fclose(fp);
        return NULL;
    }
    mp->stream = fp;
    return mp;
}

int minitar_close(struct minitar* mp)
{
    int rc = fclose(mp->stream);
    free(mp);
    if (rc) return rc;
    return 0;
}

static struct minitar_entry* minitar_attempt_read_entry(struct minitar* mp, int* valid)
{
    struct minitar_entry entry;
    struct tar_header hdr;
    if (!minitar_read_header(mp, &hdr))
    {
        *valid = 1; // we are at end-of-file
        return NULL;
    }
    if (!minitar_validate_header(&hdr))
    {
        *valid = 0;
        return NULL;
    }
    *valid = 1;
    minitar_parse_tar_header(&hdr, &entry.metadata);
    char* buf = minitar_read_file_contents(&entry.metadata, mp);
    if (!buf) return NULL;
    entry.ptr = buf;
    return minitar_dup_entry(&entry);
}

struct minitar_entry* minitar_read_entry(struct minitar* mp)
{
    int valid;
    struct minitar_entry* result;
    do {
        result = minitar_attempt_read_entry(mp, &valid);
    } while (!valid);
    return result;
}

void minitar_rewind(struct minitar* mp)
{
    rewind(mp->stream);
}

void minitar_free_entry(struct minitar_entry* entry)
{
    free(entry->ptr);
    free(entry);
}

struct minitar_entry* minitar_find_by_name(struct minitar* mp, const char* name)
{
    struct minitar_entry* entry;
    do {
        entry = minitar_read_entry(mp);
        if (entry)
        {
            if (!strcmp(entry->metadata.name, name)) return entry;
            minitar_free_entry(entry);
        }
    } while (entry);
    return NULL;
}

struct minitar_entry* minitar_find_any_of(struct minitar* mp, enum minitar_file_type type)
{
    struct minitar_entry* entry;
    do {
        entry = minitar_read_entry(mp);
        if (entry)
        {
            if (entry->metadata.type == type) return entry;
            minitar_free_entry(entry);
        }
    } while (entry);
    return NULL;
}