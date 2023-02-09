/*
 * Copyright (c) 2023, apio.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * list.c: Example utility which lists files in a tar archive.
 */

#include <minitar.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        return 1;
    }
    struct minitar mp;
    if (minitar_open(argv[1], &mp) != 0)
    {
        perror(argv[1]);
        return 1;
    }
    struct minitar_entry entry;
    do {
        if (minitar_read_entry(&mp, &entry) == 0) { printf("%s (%s, %zu bytes, mode %o)\n", entry.metadata.path, entry.metadata.name, entry.metadata.size, entry.metadata.mode); }
        else
            break;
    } while (1);
    minitar_close(&mp);
}
