#ifndef MINITAR_TAR_H
#define MINITAR_TAR_H

// Format of a raw standard tar header.

#pragma pack(push, 1)

struct tar_header
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];

    char padding[12]; // Not part of the header, only used to make the structure 512 bytes
};

#pragma pack(pop)

#endif
