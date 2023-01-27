/*
 * Copyright (c) 2023, apio.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * untar.c: Example utility which extracts files from a tar archive (POSIX only).
 */

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <fcntl.h>
#include <minitar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int untar_file(const struct minitar_entry* entry, const void* buf)
{
    int fd = open(entry->metadata.path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (fd < 0) return 1;

    if (write(fd, buf, entry->metadata.size) < 0) return 1;

    if (fchmod(fd, entry->metadata.mode) < 0) return 1;

    return close(fd);
}

static int untar_directory(const struct minitar_entry* entry)
{
    if (mkdir(entry->metadata.path, entry->metadata.mode) < 0) return 1;

    return 0;
}

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
        if (minitar_read_entry(&mp, &entry) == 0)
        {
            if (entry.metadata.type == MTAR_DIRECTORY)
            {
                int status = untar_directory(&entry);
                if (status != 0)
                {
                    fprintf(stderr, "Failed to create directory %s: %s\n", entry.metadata.path, strerror(errno));
                    break;
                }

                printf("mkdir %s\n", entry.metadata.path);
            }
            else if (entry.metadata.type == MTAR_REGULAR)
            {
                char* ptr = (char*)malloc(entry.metadata.size);
                if (!ptr)
                {
                    perror("malloc");
                    break;
                }

                minitar_read_contents(&mp, &entry, ptr, entry.metadata.size);

                int status = untar_file(&entry, ptr);

                free(ptr);

                if (status != 0)
                {
                    fprintf(stderr, "Failed to extract file %s: %s\n", entry.metadata.path, strerror(errno));
                    break;
                }

                printf("untar %s\n", entry.metadata.path);
            }
            else if (entry.metadata.type == MTAR_SYMLINK)
            {
                int status = symlink(entry.metadata.link, entry.metadata.path);

                if (status != 0)
                {
                    fprintf(stderr, "Failed to create symlink %s: %s\n", entry.metadata.path, strerror(errno));
                    break;
                }

                printf("symlink %s -> %s\n", entry.metadata.path, entry.metadata.link);
            }
            else if (entry.metadata.type == MTAR_HARDLINK)
            {
                int status = link(entry.metadata.link, entry.metadata.path);

                if (status != 0)
                {
                    fprintf(stderr, "Failed to create hard link %s: %s\n", entry.metadata.path, strerror(errno));
                    break;
                }

                printf("link %s -> %s\n", entry.metadata.path, entry.metadata.link);
            }
            else if (entry.metadata.type == MTAR_FIFO)
            {
                int status = mknod(entry.metadata.path, entry.metadata.mode | S_IFIFO, 0);

                if (status != 0)
                {
                    fprintf(stderr, "Failed to create FIFO %s: %s\n", entry.metadata.path, strerror(errno));
                    break;
                }

                printf("fifo %s\n", entry.metadata.path);
            }
        }
        else
            break;
    } while (1);
    minitar_close(&mp);
}