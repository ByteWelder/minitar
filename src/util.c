#define _IN_MINITAR
#include "minitar.h"
#include "tar.h"
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#ifdef _MSC_VER
#define strdup(p) _strdup(p)
#endif

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

// Safer BSD-style replacement for strcpy/strncpy. Copies at most size-1 bytes from src into dest, always
// null-terminating the result. Returns the full length of src, to make it easy to check for overflows. Non-standard, so
// we provide our own implementation.
// https://linux.die.net/man/3/strlcpy
static size_t minitar_strlcpy(char* dest, const char* src, size_t size)
{
    size_t len, full_len; // full_len is the total length of src, len is the length we're copying
    len = full_len = strlen(src);
    if (size == 0) return len;
    if (len > (size - 1)) len = size - 1;
    for (size_t i = 0; i < len; ++i) { *(dest + i) = *(src + i); }
    dest[len] = 0; // null-terminate
    return full_len;
}

// strdup() but copies at most max bytes of orig, always null-terminating the result. This function is non-standard and
// as such, we provide our own implementation, to be as portable as possible.
// https://linux.die.net/man/3/strndup
static char* minitar_strndup(const char* orig, size_t max)
{
    size_t len = strnlen(orig, max);
    char* ptr =
        calloc(len + 1, 1); // Use calloc so everything is automatically zeroed and we get a null-terminator for free :)
    if (!ptr) return NULL;
    for (size_t i = 0; i < len; ++i) { *(ptr + i) = *(orig + i); }
    return ptr;
}

// strcat, but for characters :)
static void minitar_append_char(char* str, char c)
{
    size_t len = strlen(str);
    str[len] = c;
    str[len + 1] = 0;
}

static size_t minitar_is_aligned_to_block_size(size_t size)
{
    return (size % 512 == 0);
}

static size_t minitar_align_down_to_block_size(size_t size)
{
    return size - (size % 512);
}

size_t minitar_align_up_to_block_size(size_t size)
{
    return minitar_is_aligned_to_block_size(size) ? size : minitar_align_down_to_block_size(size) + 512;
}

void minitar_parse_metadata_from_tar_header(const struct tar_header* hdr, struct minitar_entry_metadata* metadata)
{
    if (!strlen(hdr->prefix)) // If prefix is null, the full path is only the "name" field of the tar header.
        minitar_strlcpy(
            metadata->path, hdr->name,
            101); // We use 101 instead of 100 so that we copy the full "name" field even if it is not null-terminated.
    else // Construct the path by first taking the "prefix" field, then adding a slash, then concatenating the "name"
         // field.
    {
        minitar_strlcpy(metadata->path, hdr->prefix, 155);
        minitar_append_char(metadata->path, '/');
        strncat(metadata->path, hdr->name, 100);
        metadata->path[256] = '\0';
    }

    char* mut_path = strdup(metadata->path); // basename modifies the string passed to it, so we need to make a copy.
    if (!mut_path) minitar_panic("Failed to allocate memory");

    char* bname = basename(mut_path);

    minitar_strlcpy(metadata->name, bname, sizeof(metadata->name));
    free(mut_path);

    // Numeric fields in tar archives are stored as octal-encoded ASCII strings. Weird decision (supposedly for
    // portability), which means we have to parse these strings (the size and mtime fields aren't even null-terminated!)
    // to get the far more user-friendlier integer values stored in our metadata structure.

    metadata->mode = (mode_t)strtoul(hdr->mode, NULL, 8);
    metadata->uid = (uid_t)strtoul(hdr->uid, NULL, 8);
    metadata->gid = (gid_t)strtoul(hdr->gid, NULL, 8);

    char* sizeptr = minitar_strndup(
        hdr->size, 12); // The hdr->size field is not null-terminated, yet strndup returns a null-terminated string.
    if (!sizeptr) minitar_panic("Failed to allocate memory");
    metadata->size = (size_t)strtoull(sizeptr, NULL, 8);
    free(sizeptr);

    char* timeptr = minitar_strndup(
        hdr->mtime, 12); // The hdr->mtime field is not null-terminated, yet strndup returns a null-terminated string.
    if (!timeptr) minitar_panic("Failed to allocate memory");
    metadata->mtime = (time_t)strtoull(timeptr, NULL, 8);
    free(timeptr);

    // The type is stored as a character instead of an integer.

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

// Create a heap-allocated copy of an entry on the stack.
struct minitar_entry* minitar_dup_entry(const struct minitar_entry* original)
{
    struct minitar_entry* new = malloc(sizeof *original);
    if (!new) return NULL;
    memcpy(new, original, sizeof *new);
    return new;
}