#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

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
        int32_t sectorNum;      // �ļ�ϵͳ����������
        int32_t inodeNum;       // �ļ�ϵͳ��inode����
        int32_t blockNum;       // �ļ�ϵͳ��data block����
        int32_t availInodeNum;  // �ļ�ϵͳ�п���inode����
        int32_t availBlockNum;  // �ļ�ϵͳ�п���data block����
        int32_t blockSize;      // ÿ��block�����ֽ���
        int32_t inodesPerGroup;  // ÿ��group����inode��
        int32_t blocksPerGroup;  // ÿ��group����data block��
    };
};

union GroupDesc {  // Group Descriptor Table�ı���
    uint8_t byte[GROUP_DESC_SIZE];
    struct {
        int32_t inodeBitmap;  // ��group��inodeBitmap��ƫ����
        int32_t blockBitmap;  // ��group��blockBitmap��ƫ����
        int32_t inodeTable;   // ��group��inodeTable��ƫ����
        int32_t blockTable;   // ��group��blockTable��ƫ����
        int32_t availInodeNum;  // ��group�п���inode����
        int32_t availBlockNum;  // ��group�п���data block����
    };
};

#define INODE_TYPE_DIR 0x1
#define INODE_TYPE_FILE 0X2

union Inode {  // Inode Table�ı���
    uint8_t byte[INODE_SIZE];
    struct {
        int16_t type;  // ���ļ������͡��ô����Ƶ�
        int16_t linkCount;   // ���ļ���������
        int32_t blockCount;  // ���ļ���data block����
        int32_t size;        // ���ļ������ֽ���
        int32_t pointer[POINTER_NUM];  // data blockƫ����
        int32_t singlyPointer;  // һ��data blockƫ��������
        int32_t doublyPointer;  // ����data blockƫ��������
        int32_t triplyPointer;  // ����data blockƫ��������
    };
};

union DirEntry {  // Ŀ¼�ļ��ı���
    uint8_t byte[DIRENTRY_SIZE];
    struct {
        int32_t inode;  // ��Ŀ¼����Ӧ��inode��ƫ����
        char name[DIRENT_NAME_LENGTH];  // ��Ŀ¼����Ӧ���ļ���
    };
};

uint8_t disk[1024 * SECTOR_SIZE];
// sector 0: bootloader
// sector 1 - 200: kernel
// sector 201 - 844: fs
// sector 845 - 1024: padding

union SuperBlock g_superBlock;  // super block buffer
union GroupDesc g_groupDesc;    // block group buffer
uint8_t inodeBitmap[SECTOR_SIZE];
uint8_t blockBitmap[SECTOR_SIZE];

void readSect(void *dst, int sectorNum) {  // reading one sector of disk
    if (sectorNum < 201) {
        printf("%d\n", sectorNum);
        assert(0);
    }
    memcpy(dst, disk + sectorNum * SECTOR_SIZE, SECTOR_SIZE);
}

void writeSect(void *src, int sectorNum) {
    if (sectorNum < 201) {
        printf("%d\n", sectorNum);
        assert(0);
    }
    memcpy(disk + sectorNum * SECTOR_SIZE, src, SECTOR_SIZE);
}

void saveDisk(char *path) {
    FILE *fd = fopen(path, "wb");
    fwrite(disk, sizeof(disk), 1, fd);
    fclose(fd);
}

void formatDisk() {
    // load bootloader and kernel

    // format super block
    g_superBlock.sectorNum = 1024;
    g_superBlock.inodeNum = 512;
    g_superBlock.blockNum = 512;
    g_superBlock.availInodeNum = 512;
    g_superBlock.availBlockNum = 512;
    g_superBlock.blockSize = SECTOR_SIZE;
    g_superBlock.inodesPerGroup = 512;
    g_superBlock.blocksPerGroup = 512;

    // format group descriptor
    g_groupDesc.inodeBitmap = 203;
    g_groupDesc.blockBitmap = 204;
    g_groupDesc.inodeTable = 205;
    g_groupDesc.blockTable = 333;
    g_groupDesc.availInodeNum = 512;
    g_groupDesc.availBlockNum = 512;

    // format bitmap
    memset(inodeBitmap, 0, SECTOR_SIZE);
    memset(blockBitmap, 0, SECTOR_SIZE);

    // format root folder, inode num: 0
    inodeBitmap[0] = 1;
    blockBitmap[0] = 1;
    g_superBlock.availInodeNum--;
    g_superBlock.availBlockNum--;
    g_groupDesc.availInodeNum--;
    g_groupDesc.availBlockNum--;
    writeSect((void *)inodeBitmap, 203);
    writeSect((void *)blockBitmap, 204);
    writeSect((void *)&g_superBlock, 201);
    writeSect((void *)&g_groupDesc, 202);

    union Inode *p_inode = (union Inode *)&disk[205 * SECTOR_SIZE];
    p_inode->type = INODE_TYPE_DIR;
    p_inode->linkCount = 1;
    p_inode->blockCount = 1;
    p_inode->size = 2;  // two dir entries
    p_inode->pointer[0] = 333;
    p_inode->pointer[1] = -1;
    p_inode->singlyPointer = -1;
    p_inode->doublyPointer = -1;
    p_inode->triplyPointer = -1;

    union DirEntry *p_dirent = (union DirEntry *)&disk[333 * SECTOR_SIZE];
    p_dirent->inode = 0;
    strncpy(p_dirent->name, ".", DIRENT_NAME_LENGTH);
    p_dirent++;
    p_dirent->inode = 0;
    strncpy(p_dirent->name, "..", DIRENT_NAME_LENGTH);
}

uint32_t allocNewInode() {
    readSect(inodeBitmap, 203);
    int i;
    for (i = 0; i < 512; i++) {
        if (inodeBitmap[i] == 0) {
            inodeBitmap[i] = 1;
            writeSect(inodeBitmap, 203);
            printf("[debug] alloc inode %d\n", i);
            return i;
        }
    }
    return -1;
}

void freeInode(uint32_t inode) {
    readSect(inodeBitmap, 203);
    inodeBitmap[inode] = 0;
    writeSect(inodeBitmap, 203);
    printf("[debug] free inode %d\n", inode);
}

uint32_t allocNewBlock() {
    readSect(blockBitmap, 204);
    int i;
    for (i = 0; i < 512; i++) {
        if (blockBitmap[i] == 0) {
            blockBitmap[i] = 1;
            writeSect(blockBitmap, 204);
            printf("[debug] alloc block %d\n", i + 333);
            return i + 333;
        }
    }
    return -1;
}

void freeBlock(uint32_t blk) {
    readSect(blockBitmap, 203);
    blockBitmap[blk] = 0;
    writeSect(blockBitmap, 203);
    printf("[debug] free block %d\n", blk);
}

int32_t find_file_inode(const char *path, char *file_name) {
    if (strcmp(path, "/") == 0) return 0;
    if (*path != '/') return -1;
    char pathBuf[128];
    strncpy(pathBuf, path + 1, 128);

    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorDirentBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;
    union DirEntry *p_dirent = NULL;
    int i = 0, j = 0;
    int direntCount;
    int isFound = 0;

    char *s = strtok(pathBuf, "/");

    while (s != NULL) {
        printf("[debug] %s\n", s);
        readSect(sectorInodeBuf, 205 + inodeNum / 4);
        p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;

        direntCount = p_inode->size;

        i = j = 0;
        isFound = 0;
        ;
        while (p_inode->pointer[i] != -1) {
            readSect(sectorDirentBuf, p_inode->pointer[i]);

            p_dirent = (union DirEntry *)sectorDirentBuf;
            for (; j < direntCount; j++) {
                if (strcmp(s, p_dirent->name) == 0) {
                    strcpy(file_name, s);
                    inodeNum = p_dirent->inode;
                    isFound = 1;
                    break;
                }
                p_dirent++;
            }
            if (isFound) break;
            i++;
        }
        if (!isFound) {
            strcpy(file_name, s);
            return inodeNum;
        }
        s = strtok(NULL, "/");
    }
    return inodeNum;
}

// Upon successful completion, mkdir() shall return 0. Otherwise, -1 shall be
// returned, no directory shall be created, and errno shall be set to indicate
// the error.
int fs_mkdir(const char *path) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorDirentBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;
    union DirEntry *p_dirent = NULL;
    int direntCount;
    int32_t newInode;

    char file_name[128];
    inodeNum = find_file_inode(path, file_name);

    readSect(sectorInodeBuf, 205 + inodeNum / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;
    direntCount = p_inode->size;

    readSect(sectorDirentBuf, p_inode->pointer[0]);

    p_dirent = (union DirEntry *)sectorDirentBuf;
    p_dirent += direntCount;

    // update parent dir entries
    newInode = allocNewInode();
    p_dirent->inode = newInode;
    strncpy(p_dirent->name, file_name, DIRENT_NAME_LENGTH);
    writeSect(sectorDirentBuf, p_inode->pointer[0]);

    // update parent inode
    p_inode->size++;
    writeSect(sectorInodeBuf, 205 + inodeNum / 4);

    // update new inode
    readSect(sectorInodeBuf, 205 + newInode / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + newInode % 4;
    p_inode->type = INODE_TYPE_DIR;
    p_inode->linkCount = 1;
    p_inode->blockCount = 1;
    p_inode->size = 2;  // two dir entries
    p_inode->pointer[0] = allocNewBlock();
    p_inode->pointer[1] = -1;
    p_inode->singlyPointer = -1;
    p_inode->doublyPointer = -1;
    p_inode->triplyPointer = -1;
    writeSect(sectorInodeBuf, 205 + newInode / 4);

    // update new dir entries
    readSect(sectorDirentBuf, p_inode->pointer[0]);
    p_dirent = (union DirEntry *)sectorDirentBuf;
    p_dirent->inode = newInode;
    strncpy(p_dirent->name, ".", DIRENT_NAME_LENGTH);
    p_dirent++;
    p_dirent->inode = inodeNum;
    strncpy(p_dirent->name, "..", DIRENT_NAME_LENGTH);
    writeSect(sectorDirentBuf, p_inode->pointer[0]);

    return 0;
}

int fs_ls(const char *path) {
    char pathBuf[128];
    strncpy(pathBuf, path + 1, 128);
    printf("[debug] ls - %s\n", path);

    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorInodeBuf2[SECTOR_SIZE];
    uint8_t sectorDirentBuf[SECTOR_SIZE];
    int32_t parentInode = 0, inodeNum = 0;
    union Inode *p_inode = NULL, *p_inode2 = NULL;
    union DirEntry *p_dirent = NULL;
    int i = 0, j = 0;
    int direntCount;
    int isFound = 0;

    char *s = strtok(pathBuf, "/");

    while (s != NULL) {
        readSect(sectorInodeBuf, 205 + inodeNum / 4);
        p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;

        direntCount = p_inode->size;

        i = j = 0;
        isFound = 0;
        ;
        while (p_inode->pointer[i] != -1) {
            readSect(sectorDirentBuf, p_inode->pointer[i]);

            p_dirent = (union DirEntry *)sectorDirentBuf;
            for (; j < direntCount; j++) {
                if (strcmp(s, p_dirent->name) == 0) {
                    parentInode = inodeNum;
                    inodeNum = p_dirent->inode;
                    isFound = 1;
                    break;
                }
                p_dirent++;
            }
            if (isFound) break;
            i++;
        }
        if (!isFound) {
            return -1;
        }
        s = strtok(NULL, "/");
    }

    readSect(sectorInodeBuf, 205 + inodeNum / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;
    direntCount = p_inode->size;
    i = j = 0;
    while (p_inode->pointer[i] != -1) {
        readSect(sectorDirentBuf, p_inode->pointer[i]);

        p_dirent = (union DirEntry *)sectorDirentBuf;

        for (; j < direntCount; j++) {
            readSect(sectorInodeBuf2, 205 + (p_dirent->inode) / 4);
            p_inode2 = ((union Inode *)sectorInodeBuf2) + (p_dirent->inode) % 4;
            if (p_inode2->type == INODE_TYPE_DIR) {
                printf("dir  ");
                printf("%5d entries   ", p_inode2->size);
            } else {
                printf("file ");
                printf("%5d   bytes   ", p_inode2->size);
            }

            printf("%s%s\n", path, p_dirent->name);

            p_dirent++;
        }
        if (isFound) break;
        i++;
    }
}

int fs_create_file(const char *path) {
    if (*path != '/') return -1;
    char pathBuf[128];
    strncpy(pathBuf, path + 1, 128);

    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorDirentBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;
    union DirEntry *p_dirent = NULL;
    int i = 0, j = 0;
    int direntCount;
    int isFound = 0;

    int newInode;

    char *s = strtok(pathBuf, "/");

    while (s != NULL) {
        readSect(sectorInodeBuf, 205 + inodeNum / 4);
        p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;

        direntCount = p_inode->size;

        i = j = 0;
        isFound = 0;
        ;
        while (p_inode->pointer[i] != -1) {
            readSect(sectorDirentBuf, p_inode->pointer[i]);

            p_dirent = (union DirEntry *)sectorDirentBuf;
            for (; j < direntCount; j++) {
                if (strcmp(s, p_dirent->name) == 0) {
                    inodeNum = p_dirent->inode;
                    isFound = 1;
                    break;
                }
                p_dirent++;
            }
            if (isFound) break;
            i++;
        }
        if (!isFound) {
            newInode = allocNewInode();
            p_dirent->inode = newInode;
            strncpy(p_dirent->name, s, DIRENT_NAME_LENGTH);
            writeSect(sectorDirentBuf, p_inode->pointer[i - 1]);

            readSect(sectorInodeBuf, 205 + inodeNum / 4);
            p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;
            p_inode->size++;
            writeSect(sectorInodeBuf, 205 + inodeNum / 4);

            readSect(sectorInodeBuf, 205 + newInode / 4);
            p_inode = ((union Inode *)sectorInodeBuf) + newInode % 4;
            p_inode->type = INODE_TYPE_FILE;
            p_inode->linkCount = 1;
            p_inode->blockCount = 1;
            p_inode->size = 0;  // two dir entries
            p_inode->pointer[0] = allocNewBlock();
            p_inode->pointer[1] = -1;
            p_inode->singlyPointer = -1;
            p_inode->doublyPointer = -1;
            p_inode->triplyPointer = -1;
            writeSect(sectorInodeBuf, 205 + newInode / 4);

            return 0;
        }
        s = strtok(NULL, "/");
    }
    return 0;
}

// file should exists
size_t fs_write(const char *path, uint8_t *buf, size_t count, int offset) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorBlockBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;
    union DirEntry *p_dirent = NULL;
    int data_blk_index;
    int block_num, new_block_num;
    uint8_t *old_buf = buf;
    int i;

    char file_name[128];
    inodeNum = find_file_inode(path, file_name);

    readSect(sectorInodeBuf, 205 + inodeNum / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;
    block_num = (p_inode->size - 1) / SECTOR_SIZE + 1;
    new_block_num = (offset + count - 1) / SECTOR_SIZE + 1;
    if (new_block_num > block_num) {
        for (i = block_num; i < new_block_num; i++) {
            p_inode->pointer[i] = allocNewBlock();
        }
    } else if (new_block_num < block_num) {
        for (i = block_num; i < new_block_num; i++) {
            freeBlock(p_inode->pointer[i]);
        }
    }

    // update inode
    p_inode->pointer[new_block_num] = -1;
    if (p_inode->size < offset + count) p_inode->size = offset + count;
    writeSect(sectorInodeBuf, 205 + inodeNum / 4);

    data_blk_index = offset / SECTOR_SIZE;
    offset = offset % SECTOR_SIZE;
    while (count > 0) {
        readSect(sectorBlockBuf, p_inode->pointer[data_blk_index]);
        if (count + offset > SECTOR_SIZE) {
            memcpy(sectorBlockBuf + offset, buf, SECTOR_SIZE - offset);
            buf += (SECTOR_SIZE - offset);
            count -= (SECTOR_SIZE - offset);
            offset = 0;
        } else {
            memcpy(sectorBlockBuf + offset, buf, count);
            buf += count;
            count = 0;
            offset = 0;
        }
        writeSect(sectorBlockBuf, p_inode->pointer[data_blk_index]);
        data_blk_index++;
    }

    printf("[debug] write %d bytes\n", buf - old_buf);
    return buf - old_buf;
}

int fs_read(const char *path, uint8_t *buf, size_t count, int offset) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorBlockBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;
    union DirEntry *p_dirent = NULL;
    int data_blk_index;
    uint8_t *old_buf = buf;

    char file_name[128];
    inodeNum = find_file_inode(path, file_name);

    readSect(sectorInodeBuf, 205 + inodeNum / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;

    data_blk_index = offset / SECTOR_SIZE;
    offset = offset % SECTOR_SIZE;
    while (count > 0) {
        readSect(sectorBlockBuf, p_inode->pointer[data_blk_index]);
        if (count + offset > SECTOR_SIZE) {
            memcpy(buf, sectorBlockBuf + offset, SECTOR_SIZE - offset);
            buf += (SECTOR_SIZE - offset);
            count -= (SECTOR_SIZE - offset);
            offset = 0;
        } else {
            memcpy(buf, sectorBlockBuf + offset, count);
            buf += count;
            count = 0;
            offset = 0;
        }
        data_blk_index++;
    }

    printf("[debug] read %d bytes\n", buf - old_buf);
    return buf - old_buf;
}

void fs_cat(const char *path) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;
    int file_size;
    int i;

    char file_name[128];
    inodeNum = find_file_inode(path, file_name);

    readSect(sectorInodeBuf, 205 + inodeNum / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;
    file_size = p_inode->size;

    printf("%s file size: %d\n", path, file_size);
    fs_read(path, sectorInodeBuf, p_inode->size, 0);
    for (i = 0; i < file_size; i++) {
        printf("%x ", sectorInodeBuf[i]);
    }
    printf("\n");
}

uint8_t bootloaderBuf[2 * SECTOR_SIZE];
int bootloader_size = 0;
uint8_t kernelBuf[200 * SECTOR_SIZE];
int kernel_size = 0;
uint8_t appBuf[1024 * SECTOR_SIZE];
uint8_t appBuf2[1024 * SECTOR_SIZE];
int app_size = 0;

int main(int argc, char **argv) {
    // clear disk
    memset(disk, 0, sizeof(disk));

    char *bootload_file, *kernel_file, *app_file, *output_file;
    FILE *fp;
    if (argc == 5) {
        bootload_file = argv[1];
        kernel_file = argv[2];
        app_file = argv[3];
        output_file = argv[4];
    } else {
        printf(
            "usage: formatDisk bootload_file_path kernel_file_path "
            "app_file_path output_file_path\n");
        return 0;
    }

    fp = fopen(bootload_file, "rb");
    if (fp != NULL) {
        bootloader_size =
            fread(bootloaderBuf, sizeof(uint8_t), 2 * SECTOR_SIZE, fp);
        printf("bootloader size: %d\n", bootloader_size);
        fclose(fp);
    }

    fp = fopen(kernel_file, "rb");
    if (fp != NULL) {
        kernel_size = fread(kernelBuf, sizeof(uint8_t), 200 * SECTOR_SIZE, fp);
        printf("kernel size: %d\n", kernel_size);
        fclose(fp);
    }

    fp = fopen(app_file, "rb");
    if (fp != NULL) {
        app_size = fread(appBuf, sizeof(uint8_t), 1024 * SECTOR_SIZE, fp);
        printf("app size: %d\n", app_size);
        fclose(fp);
    }

    memcpy(disk, bootloaderBuf, SECTOR_SIZE);
    memcpy(disk + SECTOR_SIZE, kernelBuf, 200 * SECTOR_SIZE);

    formatDisk();

    fs_mkdir("/usr");
    fs_mkdir("/boot");
    fs_mkdir("/dev");
    fs_create_file("/boot/app");
    fs_create_file("/boot/bootloader");
    fs_write("/boot/app", appBuf, app_size, 0);
    fs_write("/boot/bootloader", bootloaderBuf, 512, 0);

    // fs_ls("/boot/");

    saveDisk(output_file);
}
