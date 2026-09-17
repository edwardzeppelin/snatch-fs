#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

#define snatchFS_MAGIC 0xDEADBEEF
#define snatchFS_BLOCK_SIZE 4096
#define snatchFS_FILENAME_MAX 255
#define snatchFS_DIRECT_BLOCKS 12
#define snatchFS_INODES_PER_BLOCK (snatchFS_BLOCK_SIZE / sizeof(struct snatchfs_inode))
#define snatchFS_ROOT_INODE 0

enum {
    snatchFS_FT_REGULAR = 1,
    snatchFS_FT_DIR,
    snatchFS_FT_SYMLINK
};

struct snatchfs_superblock {
    uint32_t magic;
    uint32_t block_size;
    uint32_t fs_size;
    uint32_t inode_count;
    uint32_t free_inodes;
    uint32_t free_blocks;
    uint32_t first_inode;
    uint32_t first_data;
    uint32_t bg_count;
    char pad[snatchFS_BLOCK_SIZE - 9 * sizeof(uint32_t)];
};

struct snatchfs_inode {
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t atime;
    uint32_t links;
    uint32_t blocks;
    uint32_t direct[snatchFS_DIRECT_BLOCKS];
    uint32_t indirect;
    char pad[snatchFS_BLOCK_SIZE - (12 + snatchFS_DIRECT_BLOCKS) * sizeof(uint32_t)];
};

struct snatchfs_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
};

void write_superblock(int fd, uint32_t block_count);
void write_block_bitmap(int fd, uint32_t block_count);
void write_inode_bitmap(int fd);
void write_inode_table(int fd);
void write_root_directory(int fd);
void write_block(int fd, void *data, size_t size);
void write_zeros(int fd, size_t count);
uint32_t calculate_bitmap_size(uint32_t bits);
void die(const char *msg);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <device> <size_in_MB>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *device = argv[1];
    int size_mb = atoi(argv[2]);

    if (size_mb < 1) {
        fprintf(stderr, "Error: Size must be at least 1MB\n");
        return EXIT_FAILURE;
    }

    uint32_t block_count = (size_mb * 1024 * 1024) / snatchFS_BLOCK_SIZE;
    if (block_count < 10) {
        fprintf(stderr, "Error: Filesystem too small (minimum 10 blocks required)\n");
        return EXIT_FAILURE;
    }

    printf("Creating snatchFS filesystem on %s, size %d MB (%u blocks)\n", 
           device, size_mb, block_count);

    int fd = open(device, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        die("Failed to open device");
    }

    // 1. Записываем суперблок
    write_superblock(fd, block_count);

    // 2. Битовая карта блоков
    write_block_bitmap(fd, block_count);

    // 3. Битовая карта inode
    write_inode_bitmap(fd);

    // 4. Таблица inode
    write_inode_table(fd);

    // 5. Корневой каталог
    write_root_directory(fd);

    // 6. Оставшееся пространство заполняем нулями
    off_t current_pos = lseek(fd, 0, SEEK_CUR);
    off_t fs_size = (off_t)block_count * snatchFS_BLOCK_SIZE;
    if (current_pos < fs_size) {
        write_zeros(fd, fs_size - current_pos);
    }

    close(fd);
    printf("snatchFS filesystem created successfully\n");
    return EXIT_SUCCESS;
}

void write_superblock(int fd, uint32_t block_count) {
    struct snatchfs_superblock sb = {
        .magic = snatchFS_MAGIC,
        .block_size = snatchFS_BLOCK_SIZE,
        .fs_size = block_count,
        .inode_count = 1024,
        .free_inodes = 1023, // минус корневой inode
        .free_blocks = block_count - 4, // минус суперблок, битовые карты и таблица inode
        .first_inode = 3,
        .first_data = 4,
        .bg_count = 1
    };

    if (write(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        die("Failed to write superblock");
    }

    // дополняем до размера блока
    write_zeros(fd, snatchFS_BLOCK_SIZE - sizeof(sb));
}

void write_block_bitmap(int fd, uint32_t block_count) {
    uint32_t bitmap_size = calculate_bitmap_size(block_count);
    uint8_t *bitmap = calloc(1, bitmap_size);
    if (!bitmap) {
        die("Failed to allocate block bitmap");
    }

    // первые 4 блока как занятые (суперблок, битовые карты, таблица inode)
    for (int i = 0; i < 4; i++) {
        bitmap[i / 8] |= (1 << (i % 8));
    }

    if (write(fd, bitmap, bitmap_size) != bitmap_size) {
        free(bitmap);
        die("Failed to write block bitmap");
    }

    free(bitmap);

    // дополняем до размера блока
    write_zeros(fd, snatchFS_BLOCK_SIZE - bitmap_size);
}

void write_inode_bitmap(int fd) {
    uint32_t inode_count = 1024;
    uint32_t bitmap_size = calculate_bitmap_size(inode_count);
    uint8_t *bitmap = calloc(1, bitmap_size);
    if (!bitmap) {
        die("Failed to allocate inode bitmap");
    }

    // помечаем корневой inode (0) как занятый
    bitmap[0] |= 1;

    if (write(fd, bitmap, bitmap_size) != bitmap_size) {
        free(bitmap);
        die("Failed to write inode bitmap");
    }

    free(bitmap);

    // дополняем до размера блока
    write_zeros(fd, snatchFS_BLOCK_SIZE - bitmap_size);
}

void write_inode_table(int fd) {
    struct snatchfs_inode *inodes = calloc(snatchFS_INODES_PER_BLOCK, sizeof(struct snatchfs_inode));
    if (!inodes) {
        die("Failed to allocate inode table");
    }

    // создаем корневой inode
    time_t now = time(NULL);
    inodes[snatchFS_ROOT_INODE] = (struct snatchfs_inode){
        .mode = S_IFDIR | 0755,
        .uid = getuid(),
        .gid = getgid(),
        .size = snatchFS_BLOCK_SIZE, // размер каталога (1 блок)
        .ctime = now,
        .mtime = now,
        .atime = now,
        .links = 2, // '.' и '..'
        .blocks = 1,
        .direct = {4}, // первый блок данных - корневой каталог
        .indirect = 0
    };

    if (write(fd, inodes, snatchFS_BLOCK_SIZE) != snatchFS_BLOCK_SIZE) {
        free(inodes);
        die("Failed to write inode table");
    }

    free(inodes);
}

void write_root_directory(int fd) {
    // переходим к первому блоку данных (блок 4)
    if (lseek(fd, 4 * snatchFS_BLOCK_SIZE, SEEK_SET) == -1) {
        die("Failed to seek to data block");
    }

    // создаем записи для '.' и '..'
    struct {
        struct snatchfs_dir_entry dot;
        char dot_name[1];
        struct snatchfs_dir_entry dotdot;
        char dotdot_name[2];
    } entries = {
        .dot = {
            .inode = snatchFS_ROOT_INODE,
            .rec_len = sizeof(struct snatchfs_dir_entry) + 1,
            .name_len = 1,
            .file_type = snatchFS_FT_DIR
        },
        .dot_name = ".",
        .dotdot = {
            .inode = snatchFS_ROOT_INODE,
            .rec_len = snatchFS_BLOCK_SIZE - (sizeof(struct snatchfs_dir_entry) + 1),
            .name_len = 2,
            .file_type = snatchFS_FT_DIR
        },
        .dotdot_name = ".."
    };

    if (write(fd, &entries, sizeof(entries)) != sizeof(entries)) {
        die("Failed to write root directory entries");
    }

    // заполняем остаток блока нулями
    write_zeros(fd, snatchFS_BLOCK_SIZE - sizeof(entries));
}

void write_block(int fd, void *data, size_t size) {
    if (write(fd, data, size) != size) {
        die("Failed to write block");
    }
}

void write_zeros(int fd, size_t count) {
    char zero = 0;
    for (size_t i = 0; i < count; i++) {
        if (write(fd, &zero, 1) != 1) {
            die("Failed to write zeros");
        }
    }
}

uint32_t calculate_bitmap_size(uint32_t bits) {
    return (bits + 7) / 8;
}

void die(const char *msg) {
    fprintf(stderr, "Error: %s (%s)\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}