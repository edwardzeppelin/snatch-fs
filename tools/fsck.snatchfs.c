#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "snatchfs.h"

#define MAX_INODES 1024
#define MAX_BLOCKS (1024*1024) // 4GB при 4K блоках

int fd;
struct snatchfs_superblock sb;
uint8_t *block_bitmap;
uint8_t *inode_bitmap;
uint32_t *inode_blocks;
uint32_t *block_refs;

void read_superblock() {
    if (pread(fd, &sb, sizeof(sb), 0) != sizeof(sb)) {
        fprintf(stderr, "Error reading superblock\n");
        exit(1);
    }

    if (sb.magic != snatchFS_MAGIC) {
        fprintf(stderr, "Not a snatchFS filesystem\n");
        exit(1);
    }
}

void check_bitmaps() {
    // проверка битовых карт
    block_bitmap = malloc(sb.block_size);
    inode_bitmap = malloc(sb.block_size);

    pread(fd, block_bitmap, sb.block_size, sb.block_size * 1);
    pread(fd, inode_bitmap, sb.block_size, sb.block_size * 2);

    // проверка согласованности битовых карт
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        int used = inode_bitmap[i/8] & (1 << (i%8));
        // to do дополнительные проверки
    }

    printf("Bitmaps checked\n");
}

void check_inodes() {
    struct snatchfs_inode inode;
    inode_blocks = calloc(MAX_BLOCKS, sizeof(uint32_t));

    for (uint32_t i = 0; i < sb.inode_count; i++) {
        if (!(inode_bitmap[i/8] & (1 << (i%8))))
            continue;

        off_t pos = sb.block_size * sb.first_inode + i * sizeof(inode);
        pread(fd, &inode, sizeof(inode), pos);

        // Проверка блоков inode
        for (int j = 0; j < snatchFS_DIRECT_BLOCKS; j++) {
            if (inode.direct[j]) {
                if (inode.direct[j] >= sb.fs_size) {
                    printf("Inode %d: invalid block %d\n", i, inode.direct[j]);
                }
                inode_blocks[inode.direct[j]]++;
            }
        }

        if (inode.indirect) {
            // to do проверка косвенных блоков
        }
    }

    printf("Inodes checked\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <device>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    read_superblock();
    check_bitmaps();
    check_inodes();

    close(fd);
    free(block_bitmap);
    free(inode_bitmap);
    free(inode_blocks);

    printf("Filesystem check completed\n");
    return 0;
}