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
#include <libgen.h>
#include <minitar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

static int create_parent_recursively(const char* path)
{
    char* path_copy = strdup(path);
    if (!path_copy) return -1;

    char* parent = dirname(path_copy);

create:
    if (mkdir(parent, 0755) < 0)
    {
        if (errno == ENOENT)
        {
            create_parent_recursively(parent);
            goto create;
        }

        if (errno == EEXIST) goto success;

        free(path_copy);
        return -1;
    }

success:
    free(path_copy);
    return 0;
}

static int untar_file(const struct minitar_entry* entry, const void* buf)
{
    if (create_parent_recursively(entry->metadata.path) < 0) return 1;

    int fd = open(entry->metadata.path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (fd < 0) return 1;

    if (write(fd, buf, entry->metadata.size) < 0) return 1;

    if (fchmod(fd, entry->metadata.mode) < 0) return 1;

    return close(fd);
}

static int untar_directory(const struct minitar_entry* entry)
{
    if (create_parent_recursively(entry->metadata.path) < 0) return 1;

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
    int exit_status = 0;
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
                    exit_status = 1;
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
                    exit_status = 1;
                    break;
                }

                minitar_read_contents(&mp, &entry, ptr, entry.metadata.size);

                int status = untar_file(&entry, ptr);

                free(ptr);

                if (status != 0)
                {
                    fprintf(stderr, "Failed to extract file %s: %s\n", entry.metadata.path, strerror(errno));
                    exit_status = 1;
                    break;
                }

                printf("untar %s\n", entry.metadata.path);
            }
            else if (entry.metadata.type == MTAR_SYMLINK)
            {
                if (create_parent_recursively(entry.metadata.path) < 0) goto symlink_err;

                int status = symlink(entry.metadata.link, entry.metadata.path);

                if (status != 0)
                {
                symlink_err:
                    fprintf(stderr, "Failed to create symlink %s: %s\n", entry.metadata.path, strerror(errno));
                    exit_status = 1;
                    break;
                }

                printf("symlink %s -> %s\n", entry.metadata.path, entry.metadata.link);
            }
            else if (entry.metadata.type == MTAR_HARDLINK)
            {
                if (create_parent_recursively(entry.metadata.path) < 0) goto hardlink_err;

                int status = link(entry.metadata.link, entry.metadata.path);

                if (status != 0)
                {
                hardlink_err:
                    fprintf(stderr, "Failed to create hard link %s: %s\n", entry.metadata.path, strerror(errno));
                    exit_status = 1;
                    break;
                }

                printf("link %s -> %s\n", entry.metadata.path, entry.metadata.link);
            }
            else if (entry.metadata.type == MTAR_FIFO)
            {
                if (create_parent_recursively(entry.metadata.path) < 0) goto fifo_err;

                int status = mknod(entry.metadata.path, entry.metadata.mode | S_IFIFO, 0);

                if (status != 0)
                {
                fifo_err:
                    fprintf(stderr, "Failed to create FIFO %s: %s\n", entry.metadata.path, strerror(errno));
                    exit_status = 1;
                    break;
                }

                printf("fifo %s\n", entry.metadata.path);
            }
            else if (entry.metadata.type == MTAR_BLKDEV)
            {
                if (create_parent_recursively(entry.metadata.path) < 0) goto blkdev_err;

                int status = mknod(entry.metadata.path, entry.metadata.mode | S_IFBLK,
                                   makedev(entry.metadata.devmajor, entry.metadata.devminor));

                if (status != 0)
                {
                blkdev_err:
                    fprintf(stderr, "Failed to create block device %s: %s\n", entry.metadata.path, strerror(errno));
                    exit_status = 1;
                    break;
                }

                printf("blkdev %s (%u:%u)\n", entry.metadata.path, entry.metadata.devmajor, entry.metadata.devminor);
            }
            else if (entry.metadata.type == MTAR_CHRDEV)
            {
                if (create_parent_recursively(entry.metadata.path) < 0) goto chrdev_err;

                int status = mknod(entry.metadata.path, entry.metadata.mode | S_IFCHR,
                                   makedev(entry.metadata.devmajor, entry.metadata.devminor));

                if (status != 0)
                {
                chrdev_err:
                    fprintf(stderr, "Failed to create character device %s: %s\n", entry.metadata.path, strerror(errno));
                    exit_status = 1;
                    break;
                }

                printf("chrdev %s (%u:%u)\n", entry.metadata.path, entry.metadata.devmajor, entry.metadata.devminor);
            }
            else
            {
                fprintf(stderr, "error: unknown entry type: %d", entry.metadata.type);
                exit_status = 1;
                break;
            }
        }
        else
            break;
    } while (1);
    minitar_close(&mp);
    return exit_status;
}