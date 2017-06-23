#ifndef __X86_FS_H__
#define __X86_FS_H__

#include "common.h"
#include "device.h"
#include "x86.h"

#define SECTOR_SIZE 512
#define FS_START_SECTOR 201
#define FS_BLOCK_NUM 1024

#define SUPER_SECTOR_SIZE SECTOR_SIZE
#define SUPER_BLOCK_START FS_START_SECTOR

#define GROUP_DESC_SIZE SECTOR_SIZE
#define INODE_SIZE 128
#define POINTER_NUM 25
#define DIRENTRY_SIZE 64
#define DIRENT_NAME_LENGTH (DIRENTRY_SIZE - 4)

union SuperBlock {
    uint8_t byte[SUPER_SECTOR_SIZE];
    struct {
        int32_t sectorNum;
        int32_t inodeNum;
        int32_t blockNum;
        int32_t availInodeNum;
        int32_t availBlockNum;
        int32_t blockSize;
        int32_t inodesPerGroup;
        int32_t blocksPerGroup;
    };
};

union GroupDesc {
    uint8_t byte[GROUP_DESC_SIZE];
    struct {
        int32_t inodeBitmap;
        int32_t blockBitmap;
        int32_t inodeTable;
        int32_t blockTable;
        int32_t availInodeNum;
        int32_t availBlockNum;
    };
};

#define INODE_TYPE_DIR 0x1
#define INODE_TYPE_FILE 0X2

union Inode {
    uint8_t byte[INODE_SIZE];
    struct {
        int16_t type;
        int16_t linkCount;
        int32_t blockCount;
        int32_t size;
        int32_t pointer[POINTER_NUM];
        int32_t singlyPointer;
        int32_t doublyPointer;
        int32_t triplyPointer;
    };
};

union DirEntry {
    uint8_t byte[DIRENTRY_SIZE];
    struct {
        int32_t inode;
        char name[DIRENT_NAME_LENGTH];
    };
};

uint8_t inodeBitmap[SECTOR_SIZE];
uint8_t blockBitmap[SECTOR_SIZE];

int32_t find_file_inode(const char *path, char *file_name);
int fs_mkdir(const char *path);
int fs_ls(const char *path);
int fs_create_file(const char *path);
size_t fs_write(int32_t inode, uint8_t *buf, size_t count, int offset);
int fs_read(int32_t inode, uint8_t *buf, size_t count, int offset);
void fs_cat(const char *path);
size_t fs_file_size(const char *path);

struct FCB {
    int isFree;
    int offset;
    int inode;
};

#define NR_FCB 16

struct FCB FCB_list[NR_FCB];

int allocFCB();
int freeFCB(int id);
void init_fs();

#endif
