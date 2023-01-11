#include "minitar.h"
#include "tar.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef __TINYC__
#include <stdnoreturn.h>
#else
#define noreturn _Noreturn
#endif

#if !defined(_MSC_VER) && !defined(__TINYC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

// Default implementation for minitar_handle_panic(). Since it's declared weak, any other definition will silently
// override this one :)
WEAK noreturn void minitar_handle_panic(const char* message)
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

static char dot[] = ".";

// POSIX function to extract the basename from a path. Not present on non-POSIX, but since paths inside a tar archive
// are always POSIX (I believe?), we can use a replacement that does exactly the same thing as the original basename().
// https://linux.die.net/man/3/basename
static char* minitar_basename(char* path)
{
    // If path is NULL, or the string's length is 0, return .
    if (!path) return dot;
    size_t len = strlen(path);
    if (!len) return dot;

    // Strip trailing slashes.
    char* it = path + len - 1;
    while (*it == '/' && it != path) { it--; }
    *(it + 1) = 0;
    if (it == path) return path;

    // Return path from the first character if there are no more slashes, or from the first character after the last
    // slash.
    char* beg = strrchr(path, '/');
    if (!beg) return path;
    return beg + 1;
}

static uint64_t parse_digit(char c)
{
    return c - '0';
}

static int is_valid_octal_digit(char c)
{
    if (!isdigit(c)) return 0;
    if (parse_digit(c) >= 8ull) return 0;
    return 1;
}

static uint64_t minitar_parse_octal(const char* str)
{
    uint64_t result = 0;

    while (isspace(*str)) str++;

    while (is_valid_octal_digit(*str))
    {
        result = (result * 8ull) + parse_digit(*str);
        str++;
    }

    return result;
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

// Return a static string formed by 'size' bytes copied from str, and a null terminator. This function is useful for
// when you have a fixed-size field without a null-terminator, and you need a null-terminated string to pass to a
// library function. The pointer returned WILL be overwritten by subsequent calls to this function.
static char* minitar_static_dup(const char* str, size_t size)
{
    static char result[1024];
    assert(size < 1024);
    memcpy(result, str, size);
    result[size] = 0;
    return result;
}

size_t minitar_align_up_to_block_size(size_t size)
{
    return minitar_is_aligned_to_block_size(size) ? size : minitar_align_down_to_block_size(size) + 512;
}

static void minitar_parse_basename(const char* path, char* out, size_t max)
{
    static char mutable_path_copy[512];

    minitar_strlcpy(mutable_path_copy, path, sizeof(mutable_path_copy));

    char* bname = minitar_basename(mutable_path_copy);

    minitar_strlcpy(out, bname, max);
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

    minitar_parse_basename(metadata->path, metadata->name, sizeof(metadata->name));

    // Numeric fields in tar archives are stored as octal-encoded ASCII strings. Weird decision (supposedly for
    // portability), which means we have to parse these strings (the size and mtime fields aren't even null-terminated!)
    // to get the far more user-friendlier integer values stored in our metadata structure.

    metadata->mode = (mode_t)minitar_parse_octal(hdr->mode);
    metadata->uid = (uid_t)minitar_parse_octal(hdr->uid);
    metadata->gid = (gid_t)minitar_parse_octal(hdr->gid);

    char* sizeptr = minitar_static_dup(hdr->size, 12);
    metadata->size = (size_t)minitar_parse_octal(sizeptr);

    char* timeptr = minitar_static_dup(hdr->mtime, 12);
    metadata->mtime = (time_t)minitar_parse_octal(timeptr);

    // The type is stored as a character instead of an integer.

    switch (hdr->typeflag)
    {
    case '\0':
    case '0': metadata->type = MTAR_REGULAR; break;
    case '1': minitar_panic("Links to other files within a tar archive are unsupported");
    case '2': minitar_panic("Symbolic links are unsupported");
    case '3': minitar_panic("Character devices are unsupported");
    case '4': minitar_panic("Block devices are unsupported");
    case '5': metadata->type = MTAR_DIRECTORY; break;
    case '6': minitar_panic("FIFOs are unsupported");
    default: minitar_panic("Unknown entry type in tar header");
    }

    minitar_strlcpy(metadata->uname, hdr->uname, 32);
    minitar_strlcpy(metadata->gname, hdr->gname, 32);
}

uint32_t minitar_checksum_header(const struct tar_header* hdr)
{
    uint32_t sum = 0;
    const uint8_t* ptr = (const uint8_t*)hdr;

    // Sum up all bytes in the header, as unsigned bytes...
    while (ptr < (const uint8_t*)hdr + sizeof *hdr)
    {
        sum += *ptr;
        ptr++;
    }

    // except for the chksum field, which is treated as...
    ptr = (const uint8_t*)hdr->chksum;
    while (ptr < (const uint8_t*)hdr->chksum + sizeof hdr->chksum)
    {
        sum -= *ptr;
        ptr++;
    }

    // all blanks.
    for (size_t i = 0; i < sizeof hdr->chksum; i++) { sum += ' '; }

    return sum;
}

int minitar_validate_header(const struct tar_header* hdr)
{
#ifdef MINITAR_IGNORE_UNSUPPORTED_TYPES
    if (hdr->typeflag != '\0' && hdr->typeflag != '0' && hdr->typeflag != '5') return 0;
#else
    if (hdr->typeflag != '\0' && hdr->typeflag != '0' && hdr->typeflag != '1' && hdr->typeflag != '2' &&
        hdr->typeflag != '3' && hdr->typeflag != '4' && hdr->typeflag != '5' && hdr->typeflag != '6')
        return 0;
#endif
    // FIXME: Warn on checksum mismatch unless header is all blanks?
    if (minitar_checksum_header(hdr) != minitar_parse_octal(hdr->chksum)) return 0;
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
