#include "common.h"
#include "device.h"
#include "x86.h"

uint32_t allocNewInode() {
    readSect(inodeBitmap, 203);
    int i;
    for (i = 0; i < 512; i++) {
        if (inodeBitmap[i] == 0) {
            inodeBitmap[i] = 1;
            writeSect(inodeBitmap, 203);
            prints("[kernel] alloc inode ");
            printd(i);
            printc('\n');
            return i;
        }
    }
    return -1;
}

void freeInode(uint32_t inode) {
    readSect(inodeBitmap, 203);
    inodeBitmap[inode] = 0;
    writeSect(inodeBitmap, 203);
    // printf("[debug] free inode %d\n", inode);
}

uint32_t allocNewBlock() {
    readSect(blockBitmap, 204);
    int i;
    for (i = 0; i < 512; i++) {
        if (blockBitmap[i] == 0) {
            blockBitmap[i] = 1;
            writeSect(blockBitmap, 204);
            prints("[kernel] alloc block ");
            printd(i + 333);
            printc('\n');
            return i + 333;
        }
    }
    return -1;
}

void freeBlock(uint32_t blk) {
    readSect(blockBitmap, 203);
    blockBitmap[blk] = 0;
    writeSect(blockBitmap, 203);
    // printf("[debug] free block %d\n", blk);
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) s1++, s2++;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) dest[i] = src[i];
    dest[i] = '\0';
    return dest;
}

char *strcpy(char *dest, const char *src) { return strncpy(dest, src, 128); }

char *strchr(const char *s, int c) {
    while (*s != (char)c)
        if (!*s++) return 0;
    return (char *)s;
}

size_t strcspn(const char *s1, const char *s2) {
    size_t ret = 0;
    while (*s1)
        if (strchr(s2, *s1))
            return ret;
        else
            s1++, ret++;
    return ret;
}

size_t strspn(const char *s1, const char *s2) {
    size_t ret = 0;
    while (*s1 && strchr(s2, *s1++)) ret++;
    return ret;
}

char *strtok(char *str, const char *delim) {
    static char *p = 0;
    if (str)
        p = str;
    else if (!p)
        return 0;
    str = p + strspn(p, delim);
    p = str + strcspn(str, delim);
    if (p == str) return p = 0;
    p = *p ? *p = 0, p + 1 : 0;
    return str;
}

void *memcpy(void *dest, const void *src, size_t n) {
    char *dp = (char *)dest;
    const char *sp = (const char *)src;
    while (n--) *dp++ = *sp++;
    return dest;
}

void *memset(void *s, int c, size_t len) {
    unsigned char *dst = (unsigned char *)s;
    while (len > 0) {
        *dst = (unsigned char)c;
        dst++;
        len--;
    }
    return s;
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
                    if (file_name) {
                        strncpy(file_name, s, 128);
                    }
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
            if (file_name) {
                strncpy(file_name, s, 128);
            }
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
    // printf("[debug] ls - %s\n", path);

    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorInodeBuf2[SECTOR_SIZE];
    uint8_t sectorDirentBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
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
                prints("d---------   ");
                printd(p_inode2->size);
                prints(" entries   ");

                video_prints("d---------   ");
                video_printd(p_inode2->size);
                video_prints(" entries   ");
            } else {
                prints("f---------   ");
                printd(p_inode2->size);
                prints(" bytes   ");

                video_prints("d---------   ");
                video_printd(p_inode2->size);
                video_prints(" bytes   ");
            }

            prints(path);
            prints(p_dirent->name);
            printc('\n');

            video_prints(path);
            video_prints(p_dirent->name);
            video_printc('\n');

            p_dirent++;
        }
        if (isFound) break;
        i++;
    }
    return 0;
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

            return newInode;
        }
        s = strtok(NULL, "/");
    }
    return 0;
}

// file should exists
size_t fs_write(int32_t inode, uint8_t *buf, size_t count, int offset) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorBlockBuf[SECTOR_SIZE];
    int32_t inodeNum = inode;
    union Inode *p_inode = NULL;
    int data_blk_index;
    int block_num, new_block_num;
    uint8_t *old_buf = buf;
    int i;

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

    // printf("[debug] write %d bytes\n", buf - old_buf);
    return buf - old_buf;
}

int fs_read(int32_t inode, uint8_t *buf, size_t count, int offset) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    uint8_t sectorBlockBuf[SECTOR_SIZE];
    int32_t inodeNum = inode;
    union Inode *p_inode = NULL;
    int data_blk_index;
    uint8_t *old_buf = buf;

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

    // printf("[debug] read %d bytes\n", buf - old_buf);
    return buf - old_buf;
}

int allocFCB() {
    int i;
    for (i = 0; i < NR_FCB; i++) {
        if (FCB_list[i].isFree) {
            FCB_list[i].isFree = 0;
            return i + 3;
        }
    }
    return -1;
}

int freeFCB(int fd) {
    FCB_list[fd].isFree = 1;
    return 0;
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

    // printf("%s file size: %d\n", path, file_size);
    fs_read(find_file_inode(path, NULL), sectorInodeBuf, p_inode->size, 0);
    for (i = 0; i < file_size; i++) {
        printc(sectorInodeBuf[i]);
        video_printc(sectorInodeBuf[i]);
    }
    printc('\n');
    video_printc('\n');
}

size_t fs_file_size(const char *path) {
    uint8_t sectorInodeBuf[SECTOR_SIZE];
    int32_t inodeNum = 0;
    union Inode *p_inode = NULL;

    char file_name[128];
    inodeNum = find_file_inode(path, file_name);

    readSect(sectorInodeBuf, 205 + inodeNum / 4);
    p_inode = ((union Inode *)sectorInodeBuf) + inodeNum % 4;
    return p_inode->size;
}

void init_fs() {
    int i;
    for (i = 0; i < NR_FCB; i++) {
        FCB_list[i].isFree = 1;
    }
}
