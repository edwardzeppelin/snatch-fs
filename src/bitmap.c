#include <linux/buffer_head.h>
#include "snatchfs.h"

int snatchfs_alloc_block(struct super_block *sb)
{
    struct snatchfs_superblock *disk_sb = sb->s_fs_info;
    struct buffer_head *bh;
    uint32_t block_bitmap_block = 1;
    uint32_t blocks_count = disk_sb->fs_size;
    uint8_t *bitmap;
    uint32_t i, j;

    bh = sb_bread(sb, block_bitmap_block);
    if (!bh)
        return -EIO;

    bitmap = (uint8_t *)bh->b_data;

    for (i = 0; i < blocks_count / 8; i++) {
        if (bitmap[i] != 0xff) {
            for (j = 0; j < 8; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    bitmap[i] |= (1 << j);
                    mark_buffer_dirty(bh);
                    brelse(bh);

                    disk_sb->free_blocks--;
                    mark_buffer_dirty(sb->s_bdev->bd_super->s_bdev->bd_holder);

                    return i * 8 + j + disk_sb->first_data;
                }
            }
        }
    }

    brelse(bh);
    return -ENOSPC;
}

void snatchfs_free_block(struct super_block *sb, uint32_t block)
{
    struct snatchfs_superblock *disk_sb = sb->s_fs_info;
    struct buffer_head *bh;
    uint32_t block_bitmap_block = 1;
    uint32_t bit = block - disk_sb->first_data;
    uint8_t *bitmap;

    bh = sb_bread(sb, block_bitmap_block);
    if (!bh)
        return;

    bitmap = (uint8_t *)bh->b_data;
    bitmap[bit / 8] &= ~(1 << (bit % 8));
    mark_buffer_dirty(bh);
    brelse(bh);

    disk_sb->free_blocks++;
    mark_buffer_dirty(sb->s_bdev->bd_super->s_bdev->bd_holder);
}