#define _IN_MINITAR
#include "tar.h"
#include "minitar.h"
#include <stdlib.h>
#include <string.h>

// all of these are defined in util.c
int minitar_read_header(struct minitar*, struct tar_header*);
int minitar_validate_header(const struct tar_header*);
void minitar_parse_metadata_from_tar_header(const struct tar_header*, struct minitar_entry_metadata*);
struct minitar_entry* minitar_dup_entry(const struct minitar_entry*);
char* minitar_read_file_contents(struct minitar_entry_metadata*, struct minitar*);
size_t minitar_get_size_in_blocks(size_t);

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
    return rc;
}

static struct minitar_entry* minitar_attempt_read_entry(struct minitar* mp, int* valid)
{
    struct minitar_entry entry;
    struct tar_header hdr;
    *valid = 1;
    if (!minitar_read_header(mp, &hdr)) return NULL;
    if (!minitar_validate_header(&hdr))
    {
        *valid = 0;
        return NULL;
    }
    if (fgetpos(mp->stream, &entry.position)) return NULL;
    minitar_parse_metadata_from_tar_header(&hdr, &entry.metadata);
    if (entry.metadata.size)
    {
        size_t size_in_blocks = minitar_get_size_in_blocks(entry.metadata.size);
        if (fseek(mp->stream, size_in_blocks,
                  SEEK_CUR)) // move over to the next block, skipping over the file contents
        {
            return NULL;
        }
    }
    return minitar_dup_entry(&entry);
}

struct minitar_entry* minitar_read_entry(struct minitar* mp)
{
    int valid;
    struct minitar_entry* result;
    do {
        result = minitar_attempt_read_entry(mp, &valid);
    } while (!valid); // Skip over invalid entries
    return result;
}

void minitar_rewind(struct minitar* mp)
{
    rewind(mp->stream);
}

void minitar_free_entry(struct minitar_entry* entry)
{
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

size_t minitar_read_contents(struct minitar* mp, struct minitar_entry* entry, char* buf, size_t max)
{
    if (!max) return 0;
    fpos_t current_position;
    if (fgetpos(mp->stream, &current_position)) return 0;
    if (fsetpos(mp->stream, &entry->position)) return 0;
    size_t nread = fread(buf, 1, max > entry->metadata.size ? entry->metadata.size : max, mp->stream);
    if (ferror(mp->stream)) return 0;
    if (fsetpos(mp->stream, &current_position)) return 0;
    return nread;
}