#ifndef MINITAR_H
#define MINITAR_H
#include <stdio.h>
#include <sys/types.h>

#ifdef _IN_MINITAR
struct minitar
{
    FILE* stream;
};
#else
struct minitar;
#endif

enum minitar_file_type
{
    MTAR_REGULAR,
    MTAR_CHRDEV,
    MTAR_BLKDEV,
    MTAR_DIRECTORY
};

struct minitar_entry_metadata
{
    char path[257];
    char name[128];
    mode_t mode;
    uid_t uid;
    gid_t gid;
    size_t size;
    time_t mtime;
    enum minitar_file_type type;
    char uname[32];
    char gname[32];
};

struct minitar_entry
{
    struct minitar_entry_metadata metadata;
    fpos_t position;
};

#ifdef __cplusplus
extern "C"
{
#endif

    struct minitar* minitar_open(const char* pathname);
    struct minitar_entry* minitar_read_entry(struct minitar* mp);
    void minitar_free_entry(struct minitar_entry* entry);
    void minitar_rewind(struct minitar* mp);
    struct minitar_entry* minitar_find_by_name(struct minitar* mp, const char* name);
    struct minitar_entry* minitar_find_by_path(struct minitar* mp, const char* path);
    struct minitar_entry* minitar_find_any_of(struct minitar* mp, enum minitar_file_type type);
    int minitar_close(struct minitar* mp);
    size_t minitar_read_contents(struct minitar* mp, struct minitar_entry* entry, char* buf, size_t max);

#ifdef __cplusplus
}
#endif

#endif