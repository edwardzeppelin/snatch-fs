#ifndef snatchFS_H
#define snatchFS_H

#include <linux/types.h>
#include <linux/fs.h>
#include <stdint.h>

#define snatchFS_MAGIC 0xDEADBEEF
#define snatchFS_BLOCK_SIZE 4096
#define snatchFS_FILENAME_MAX 255
#define snatchFS_DIRECT_BLOCKS 12
#define snatchFS_INODES_PER_BLOCK (snatchFS_BLOCK_SIZE / sizeof(struct snatchfs_inode))

// Структура суперблока
struct snatchfs_superblock {
    uint32_t magic;         // Магическое число
    uint32_t block_size;    // Размер блока
    uint32_t fs_size;       // Размер ФС в блоках
    uint32_t inode_count;   // Общее количество inode
    uint32_t free_inodes;   // Свободные inode
    uint32_t free_blocks;   // Свободные блоки
    uint32_t first_inode;   // Первый блок таблицы inode
    uint32_t first_data;    // Первый блок данных
    uint32_t bg_count;      // Количество групп блоков
    char pad[snatchFS_BLOCK_SIZE - 9 * sizeof(uint32_t)]; // Дополнение
};

// Структура inode
struct snatchfs_inode {
    uint32_t mode;          // Тип и права доступа
    uint32_t uid;           // Владелец
    uint32_t gid;           // Группа
    uint32_t size;          // Размер файла
    uint32_t ctime;         // Время создания
    uint32_t mtime;         // Время модификации
    uint32_t atime;         // Время доступа
    uint32_t links;         // Количество ссылок
    uint32_t blocks;        // Используемые блоки
    uint32_t direct[snatchFS_DIRECT_BLOCKS]; // Прямые указатели
    uint32_t indirect;      // Косвенный указатель
    char pad[snatchFS_BLOCK_SIZE - (12 + snatchFS_DIRECT_BLOCKS) * sizeof(uint32_t)];
};

// Запись в каталоге
struct snatchfs_dir_entry {
    uint32_t inode;         // Номер inode
    uint16_t rec_len;       // Длина записи
    uint8_t name_len;       // Длина имени
    uint8_t file_type;      // Тип файла
    char name[];            // Имя файла (переменная длина)
};

// Типы файлов
enum {
    snatchFS_FT_REGULAR = 1,  // Обычный файл
    snatchFS_FT_DIR,          // Каталог
    snatchFS_FT_SYMLINK       // Символическая ссылка
};

#endif // snatchFS_H