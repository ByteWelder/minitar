/*
 * Copyright (c) 2022-2023, apio.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * tar.c: minitar API implementation.
 */

#include "tar.h"
#include "minitar.h"
#include <stdio.h>
#include <string.h>

#ifndef __TINYC__
#include <stdnoreturn.h>
#else
#define noreturn _Noreturn
#endif

// all of these are defined in util.c
int minitar_read_header(struct minitar*, struct tar_header*);
noreturn void minitar_handle_panic(const char*);
int minitar_validate_header(const struct tar_header*);
void minitar_parse_metadata_from_tar_header(const struct tar_header*, struct minitar_entry_metadata*);
void minitar_construct_header_from_metadata(struct tar_header*, const struct minitar_entry_metadata*);
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

int minitar_open_w(const char* pathname, struct minitar_w* out, enum minitar_write_mode mode)
{
    const char* mode_string;
    switch (mode)
    {
    case MTAR_APPEND: mode_string = "ab"; break;
    case MTAR_OVERWRITE: mode_string = "wb"; break;
    default: minitar_handle_panic("mode passed to minitar_open_w is not supported");
    }
    FILE* fp = fopen(pathname, mode_string);
    if (!fp) return -1;
    out->stream = fp;
    return 0;
}

int minitar_close(struct minitar* mp)
{
    return fclose(mp->stream);
}

int minitar_close_w(struct minitar_w* mp)
{
    return fclose(mp->stream);
}

// Try to read a valid header, and construct an entry from it. If the 512-byte block at the current read offset is not a
// valid header, valid is set to 0 so we can try again with the next block. In any other case, valid is set to 1. This
// helps distinguish valid return values, EOF, and invalid headers that we should just skip.
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
    if (fgetpos(mp->stream, &out->_internal._mt_position)) return -1;

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

int minitar_write_file_entry(struct minitar_w* mp, const struct minitar_entry_metadata* metadata, char* buf)
{
    struct minitar_entry_metadata meta = *metadata;
    meta.type = MTAR_REGULAR;

    struct tar_header hdr;
    minitar_construct_header_from_metadata(&hdr, &meta);
    // Write the header.
    size_t nwrite = fwrite(&hdr, sizeof(hdr), 1, mp->stream);
    if (nwrite == 0 && ferror(mp->stream)) return -1;

    // Write the file data.
    nwrite = fwrite(buf, 1, meta.size, mp->stream);
    if (nwrite == 0 && ferror(mp->stream)) return -1;

    char zeroes[512];
    memset(zeroes, 0, sizeof(zeroes));

    // Write as many zeroes as necessary to finish a block.
    size_t nzero = minitar_align_up_to_block_size(meta.size) - meta.size;
    nwrite = fwrite(zeroes, 1, nzero, mp->stream);
    if (nwrite == 0 && ferror(mp->stream)) return -1;

    return 0;
}

int minitar_write_special_entry(struct minitar_w* mp, const struct minitar_entry_metadata* metadata)
{
    struct minitar_entry_metadata meta = *metadata;
    if (meta.type == MTAR_REGULAR)
        minitar_handle_panic("Trying to write a special entry, yet MTAR_REGULAR passed as the entry type");
    meta.size = 0;

    struct tar_header hdr;
    minitar_construct_header_from_metadata(&hdr, &meta);
    size_t nwrite = fwrite(&hdr, sizeof(hdr), 1, mp->stream);
    if (nwrite == 0 && ferror(mp->stream)) return -1;

    return 0;
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

size_t minitar_read_contents(struct minitar* mp, const struct minitar_entry* entry, char* buf, size_t max)
{
    if (!max) return 0;
    if (!entry->metadata.size) return 0;

    fpos_t current_position;

    // Save the current position
    if (fgetpos(mp->stream, &current_position)) return 0;
    // Move to the position stored in the entry
    if (fsetpos(mp->stream, &entry->_internal._mt_position)) return 0;

    // We refuse to read more than the size indicated by the archive
    if (max > entry->metadata.size) max = entry->metadata.size;

    size_t nread = fread(buf, 1, max, mp->stream);
    if (ferror(mp->stream)) return 0;

    // Restore the current position
    if (fsetpos(mp->stream, &current_position)) return 0;

    return nread;
}
