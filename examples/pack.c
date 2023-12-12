/*
 * Copyright (c) 2023, apio.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * pack.c: Example utility which creates a tar archive (POSIX only).
 */

#define _XOPEN_SOURCE 700
#include <minitar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s [output] files\n", argv[0]);
        return 1;
    }
    struct minitar_w mp;
    if (minitar_open_w(argv[1], &mp, MTAR_OVERWRITE) != 0)
    {
        perror(argv[1]);
        return 1;
    }
    int exit_status = 0;
    int arg = 2;
    while (arg < argc)
    {
        FILE* fp = fopen(argv[arg], "r");
        if (!fp)
        {
            perror("fopen");
            exit_status = 1;
            break;
        }

        // Get the file length.
        fseek(fp, 0, SEEK_END);
        size_t length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char* buf = malloc(length);
        if (!buf)
        {
            perror("malloc");
            fclose(fp);
            exit_status = 1;
            break;
        }
        fread(buf, 1, length, fp);
        if (ferror(fp))
        {
            perror("fread");
            goto err;
        }

        struct stat st;
        int rc = fstat(fileno(fp), &st);
        if (rc < 0)
        {
            perror("fstat");
            goto err;
        }

        struct minitar_entry_metadata metadata;
        strncpy(metadata.path, argv[arg], sizeof(metadata.path));
        metadata.uid = st.st_uid;
        metadata.gid = st.st_gid;
        metadata.mtime = st.st_mtime;
        metadata.size = length;
        metadata.type = MTAR_REGULAR;
        metadata.mode = st.st_mode & ~S_IFMT;

        rc = minitar_write_file_entry(&mp, &metadata, buf);
        free(buf);
        fclose(fp);

        if (rc != 0)
        {
            perror("write entry failed");
            exit_status = 1;
            break;
        }

        arg++;
        continue;

    err:
        free(buf);
        fclose(fp);
        exit_status = 1;
        break;
    }
    minitar_close_w(&mp);
    return exit_status;
}