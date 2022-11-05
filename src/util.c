#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <string.h>
#include "minitar.h"
#include "tar.h"

noreturn void minitar_panic(const char* message)
{
    fprintf(stderr, "minitar: %s\n", message);
    abort();
}

void minitar_parse_tar_header(struct tar_header* hdr, struct minitar_entry_metadata* metadata)
{
    if(!strlen(hdr->prefix))
    {
        strncpy(metadata->name, hdr->name, 100);
        metadata->name[100] = '\0';
    } else {
        strcpy(metadata->name, hdr->prefix);
        strcat(metadata->name, "/");
        strncat(metadata->name, hdr->name, 100);
        metadata->name[256] = '\0';
    }

    metadata->mode = (mode_t)strtol(hdr->mode, NULL, 8);
    metadata->uid = (uid_t)strtoul(hdr->uid, NULL, 8);
    metadata->gid = (gid_t)strtoul(hdr->gid, NULL, 8);

    char* sizeptr = strndup(hdr->size, 12);
    if(!sizeptr) minitar_panic("Failed to allocate memory to duplicate a tar header's size field");
    metadata->size = (size_t)strtoull(sizeptr, NULL, 8);
    free(sizeptr);

    char* timeptr = strndup(hdr->mtime, 12);
    if(!timeptr) minitar_panic("Failed to allocate memory to duplicate a tar header's mtime field");
    metadata->mtime = (time_t)strtol(timeptr, NULL, 8);
    free(timeptr);

    switch(hdr->typeflag)
    {
        case '\0':
        case '0':
            metadata->type = MTAR_REGULAR;
            break;
        case '1':
            minitar_panic("Links to other files within a tar archive are unsupported");
        case '2':
            minitar_panic("Symbolic links are unsupported");
        case '3':
            metadata->type = MTAR_CHRDEV;
            break;
        case '4':
            metadata->type = MTAR_BLKDEV;
            break;
        case '5':
            metadata->type = MTAR_DIRECTORY;
            break;
        case '6':
            minitar_panic("FIFOs are unsupported");
        default:
            minitar_panic("Unknown entry type in tar header");
    }

    strcpy(metadata->uname, hdr->uname);
    strcpy(metadata->gname, hdr->gname);
}

int minitar_validate_header(struct tar_header* hdr)
{
    return !strncmp(hdr->magic, "ustar", 5);
}

int minitar_read_header(struct minitar* mp, struct tar_header* hdr)
{
    size_t rc = fread(hdr, sizeof *hdr, 1, mp->stream);
    if(rc == 0 && feof(mp->stream)) return 0;
    return 1;
}

struct minitar_entry* minitar_dup_entry(struct minitar_entry* original)
{
    struct minitar_entry* new = malloc(sizeof *original);
    if(!new) return NULL;
    memcpy(new, original, sizeof *new);
    return new;
}