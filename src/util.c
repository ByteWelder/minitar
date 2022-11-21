#define _POSIX_C_SOURCE 200809L // for strndup
#define _IN_MINITAR
#include "minitar.h"
#include "tar.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

// Default implementation for minitar_handle_panic(). Since it's declared weak, any other definition will silently
// override this one :)
__attribute__((weak)) noreturn void minitar_handle_panic(const char* message)
{
    fprintf(stderr, "minitar: %s\n", message);
    abort();
}

noreturn void minitar_panic(const char* message)
{
    minitar_handle_panic(message);
}

size_t minitar_strlcpy(char* dest, const char* src, size_t size)
{
    size_t len, full_len;
    len = full_len = strlen(src);
    if (size == 0) return len;
    if (len > (size - 1)) len = size - 1;
    for (size_t i = 0; i < len; ++i) { *((char*)dest + i) = *((const char*)src + i); }
    dest[len] = 0;
    return full_len;
}

void minitar_append_char(char* str, char c)
{
    size_t len = strlen(str);
    str[len] = c;
    str[len + 1] = 0;
}

void minitar_parse_tar_header(const struct tar_header* hdr, struct minitar_entry_metadata* metadata)
{
    if (!strlen(hdr->prefix))
    {
        minitar_strlcpy(metadata->name, hdr->name, 100);
        metadata->name[100] = '\0';
    }
    else
    {
        minitar_strlcpy(metadata->name, hdr->prefix, 155);
        minitar_append_char(metadata->name, '/');
        strncat(metadata->name, hdr->name, 100);
        metadata->name[256] = '\0';
    }

    metadata->mode = (mode_t)strtoul(hdr->mode, NULL, 8);
    metadata->uid = (uid_t)strtoul(hdr->uid, NULL, 8);
    metadata->gid = (gid_t)strtoul(hdr->gid, NULL, 8);

    char* sizeptr = strndup(
        hdr->size, 12); // The hdr->size field is not null-terminated, yet strndup returns a null-terminated string.
    if (!sizeptr) minitar_panic("Failed to allocate memory to duplicate a tar header's size field");
    metadata->size = (size_t)strtoull(sizeptr, NULL, 8);
    free(sizeptr);

    char* timeptr = strndup(
        hdr->mtime, 12); // The hdr->mtime field is not null-terminated, yet strndup returns a null-terminated string.
    if (!timeptr) minitar_panic("Failed to allocate memory to duplicate a tar header's mtime field");
    metadata->mtime = (time_t)strtoull(timeptr, NULL, 8);
    free(timeptr);

    switch (hdr->typeflag)
    {
    case '\0':
    case '0': metadata->type = MTAR_REGULAR; break;
    case '1': minitar_panic("Links to other files within a tar archive are unsupported");
    case '2': minitar_panic("Symbolic links are unsupported");
    case '3': metadata->type = MTAR_CHRDEV; break;
    case '4': metadata->type = MTAR_BLKDEV; break;
    case '5': metadata->type = MTAR_DIRECTORY; break;
    case '6': minitar_panic("FIFOs are unsupported");
    default: minitar_panic("Unknown entry type in tar header");
    }

    minitar_strlcpy(metadata->uname, hdr->uname, 32);
    minitar_strlcpy(metadata->gname, hdr->gname, 32);
}

int minitar_validate_header(const struct tar_header* hdr)
{
    if (hdr->typeflag != '\0' && hdr->typeflag != '0' && hdr->typeflag != '1' && hdr->typeflag != '2' &&
        hdr->typeflag != '3' && hdr->typeflag != '4' && hdr->typeflag != '5' && hdr->typeflag != '6')
        return 0;
    return !strncmp(hdr->magic, "ustar", 5);
}

int minitar_read_header(struct minitar* mp, struct tar_header* hdr)
{
    size_t rc = fread(hdr, 1, sizeof *hdr, mp->stream);
    if (rc == 0 && feof(mp->stream)) return 0;
    if (rc == 0 && ferror(mp->stream)) minitar_panic("Error while reading file header from tar archive");
    if (rc < sizeof *hdr) minitar_panic("Valid tar files should be split in 512-byte blocks");
    return 1;
}

struct minitar_entry* minitar_dup_entry(const struct minitar_entry* original)
{
    struct minitar_entry* new = malloc(sizeof *original);
    if (!new) return NULL;
    memcpy(new, original, sizeof *new);
    return new;
}

char* minitar_read_file_contents(struct minitar_entry_metadata* metadata, struct minitar* mp)
{
    char* buf = malloc(metadata->size + 1);
    if (!buf) return NULL;

    size_t nread = fread(buf, 1, metadata->size, mp->stream);
    if (!nread)
    {
        if (feof(mp->stream))
        {
            free(buf);
            return NULL;
        }
        if (ferror(mp->stream))
        {
            free(buf);
            minitar_panic("Error while reading file data from tar archive");
        }
    }
    else
    {
        long rem = 512 - (nread % 512);
        fseek(mp->stream, rem, SEEK_CUR); // move the file offset over to the start of the next block
    }

    buf[nread] = 0;

    return buf;
}