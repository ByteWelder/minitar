#include "tar.h"
#include "minitar.h"
#include <stdio.h>
#include <string.h>

// all of these are defined in util.c
int minitar_read_header(struct minitar*, struct tar_header*);
int minitar_validate_header(const struct tar_header*);
void minitar_parse_metadata_from_tar_header(const struct tar_header*, struct minitar_entry_metadata*);
size_t minitar_align_up_to_block_size(size_t);

int minitar_open(const char* pathname, struct minitar* out)
{
    FILE* fp =
        fopen(pathname,
              "rb"); // On some systems, opening the file in binary mode might be necessary to read the file properly.
    if (!fp) return -1;
    out->stream = fp;
    return 0;
}

int minitar_close(struct minitar* mp)
{
    return fclose(mp->stream);
}

// Try to read a valid header, and construct an entry from it. If the 512-byte block at the current read offset is not a
// valid header, valid is set to 0 so we can try again with the next block. In any other case, valid is set to 1. This
// helps distinguish valid return values, null pointers that should be returned to the user (for example, EOF), and
// invalid headers where we should just try again until we find a valid one.
static int minitar_try_to_read_valid_entry(struct minitar* mp, struct minitar_entry* out, int* valid)
{
    struct tar_header hdr;
    *valid = 1;

    if (!minitar_read_header(mp, &hdr)) return -1;
    if (!minitar_validate_header(&hdr))
    {
        *valid = 0;
        return -1;
    }

    // Fetch the current read position (which is currently pointing to the start of the entry's contents), so we can
    // return back to it when reading the contents of this entry using minitar_read_contents().
    if (fgetpos(mp->stream, &out->position)) return -1;

    minitar_parse_metadata_from_tar_header(&hdr, &out->metadata);
    if (out->metadata.size)
    {
        size_t size_in_archive = minitar_align_up_to_block_size(out->metadata.size);
        if (fseek(mp->stream, size_in_archive,
                  SEEK_CUR)) // move over to the next block, skipping over the file contents
        {
            return -1;
        }
    }

    return 0;
}

int minitar_read_entry(struct minitar* mp, struct minitar_entry* out)
{
    int valid, result;
    do {
        result = minitar_try_to_read_valid_entry(mp, out, &valid);
    } while (!valid); // Skip over invalid entries
    return result;
}

void minitar_rewind(struct minitar* mp)
{
    rewind(mp->stream);
}

int minitar_find_by_name(struct minitar* mp, const char* name, struct minitar_entry* out)
{
    int rc;
    do {
        rc = minitar_read_entry(mp, out);
        if (rc == 0)
        {
            if (!strcmp(out->metadata.name, name)) return 0;
        }
    } while (rc == 0);
    return -1;
}

int minitar_find_by_path(struct minitar* mp, const char* path, struct minitar_entry* out)
{
    int rc;
    do {
        rc = minitar_read_entry(mp, out);
        if (rc == 0)
        {
            if (!strcmp(out->metadata.path, path)) return 0;
        }
    } while (rc == 0);
    return -1;
}

int minitar_find_any_of(struct minitar* mp, enum minitar_file_type type, struct minitar_entry* out)
{
    int rc;
    do {
        rc = minitar_read_entry(mp, out);
        if (rc == 0)
        {
            if (out->metadata.type == type) return 0;
        }
    } while (rc == 0);
    return -1;
}

size_t minitar_read_contents(struct minitar* mp, struct minitar_entry* entry, char* buf, size_t max)
{
    if (!max) return 0;
    if (!entry->metadata.size) return 0;

    fpos_t current_position;

    // Save the current position
    if (fgetpos(mp->stream, &current_position)) return 0;
    // Move to the position stored in the entry
    if (fsetpos(mp->stream, &entry->position)) return 0;

    // We refuse to read more than the size indicated by the archive
    if (max > entry->metadata.size) max = entry->metadata.size;

    size_t nread = fread(buf, 1, max, mp->stream);
    if (ferror(mp->stream)) return 0;

    // Restore the current position
    if (fsetpos(mp->stream, &current_position)) return 0;

    return nread;
}