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
        if (minitar_read_entry(&mp, &entry) == 0) { printf("%s\n", entry.metadata.path); }
        else
            break;
    } while (1);
    minitar_close(&mp);
}