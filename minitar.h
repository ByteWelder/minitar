/*
 * Copyright (c) 2022-2023, apio.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * minitar.h: The minitar header.
 */

#ifndef MINITAR_H
#define MINITAR_H
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

struct minitar
{
    FILE* stream;
};

struct minitar_w
{
    FILE* stream;
};

enum minitar_write_mode
{
    MTAR_APPEND,
    MTAR_OVERWRITE
};

enum minitar_file_type
{
    MTAR_REGULAR,
    MTAR_DIRECTORY,
    MTAR_SYMLINK,
    MTAR_HARDLINK,
    MTAR_FIFO,
    MTAR_CHRDEV,
    MTAR_BLKDEV
};

struct minitar_entry_internal
{
    fpos_t _mt_position;
};

struct minitar_entry_metadata
{
    char path[257];
    char name[128];
    char link[101];
    mode_t mode;
    uid_t uid;
    gid_t gid;
    size_t size;
    time_t mtime;
    enum minitar_file_type type;
    char uname[32];
    char gname[32];
    unsigned devminor;
    unsigned devmajor;
};

struct minitar_entry
{
    struct minitar_entry_metadata metadata;
    struct minitar_entry_internal _internal;
};

#ifdef __cplusplus
extern "C"
{
#endif

    int minitar_open(const char* pathname, struct minitar* out);
    int minitar_open_w(const char* pathname, struct minitar_w* out, enum minitar_write_mode mode);
    int minitar_read_entry(struct minitar* mp, struct minitar_entry* out);
    int minitar_write_file_entry(struct minitar_w* mp, const struct minitar_entry_metadata* metadata, char* buf);
    int minitar_write_special_entry(struct minitar_w* mp, const struct minitar_entry_metadata* metadata);
    void minitar_rewind(struct minitar* mp);
    int minitar_find_by_name(struct minitar* mp, const char* name, struct minitar_entry* out);
    int minitar_find_by_path(struct minitar* mp, const char* path, struct minitar_entry* out);
    int minitar_find_any_of(struct minitar* mp, enum minitar_file_type type, struct minitar_entry* out);
    size_t minitar_read_contents(struct minitar* mp, const struct minitar_entry* entry, char* buf, size_t max);
    int minitar_close(struct minitar* mp);
    int minitar_close_w(struct minitar_w* mp);

#ifdef __cplusplus
}
#endif

#endif
