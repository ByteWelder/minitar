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
    char name[257];
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
    char* ptr;
};

struct minitar* minitar_open(const char* pathname);
struct minitar_entry* minitar_read_entry(struct minitar* mp);
void minitar_free_entry(struct minitar_entry* entry);
int minitar_close(struct minitar* mp);

#endif