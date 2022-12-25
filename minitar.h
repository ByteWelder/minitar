#ifndef MINITAR_H
#define MINITAR_H
#include <stdio.h>
#include <sys/types.h>

struct minitar
{
    FILE* stream;
};

enum minitar_file_type
{
    MTAR_REGULAR,
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

    int minitar_open(const char* pathname, struct minitar* out);
    int minitar_read_entry(struct minitar* mp, struct minitar_entry* out);
    void minitar_rewind(struct minitar* mp);
    int minitar_find_by_name(struct minitar* mp, const char* name, struct minitar_entry* out);
    int minitar_find_by_path(struct minitar* mp, const char* path, struct minitar_entry* out);
    int minitar_find_any_of(struct minitar* mp, enum minitar_file_type type, struct minitar_entry* out);
    size_t minitar_read_contents(struct minitar* mp, struct minitar_entry* entry, char* buf, size_t max);
    int minitar_close(struct minitar* mp);

#ifdef __cplusplus
}
#endif

#endif