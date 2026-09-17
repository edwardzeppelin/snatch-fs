#include <linux/buffer_head.h>
#include <linux/fs.h>
#include "snatchfs.h"

static int snatchfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head *bh;
    struct snatchfs_superblock *disk_sb;
    struct inode *root_inode;
    int ret = -EINVAL;

    bh = sb_bread(sb, 0);
    if (!bh) {
        printk(KERN_ERR "snatchFS: Cannot read superblock\n");
        return -EIO;
    }

    disk_sb = (struct snatchfs_superblock *)bh->b_data;
    if (disk_sb->magic != snatchFS_MAGIC) {
        if (!silent)
            printk(KERN_ERR "snatchFS: Wrong magic number\n");
        goto release;
    }

    sb->s_magic = disk_sb->magic;
    sb->s_fs_info = disk_sb;
    sb->s_op = &snatchfs_super_ops;
    sb->s_blocksize = disk_sb->block_size;
    sb->s_blocksize_bits = fls(disk_sb->block_size) - 1;

    root_inode = snatchfs_iget(sb, snatchFS_ROOT_INODE);
    if (IS_ERR(root_inode)) {
        ret = PTR_ERR(root_inode);
        goto release;
    }

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto release;
    }

    brelse(bh);
    return 0;

release:
    brelse(bh);
    return ret;
}

struct super_operations snatchfs_super_ops = {
    .put_super = snatchfs_put_super,
    .statfs = simple_statfs,
    .evict_inode = snatchfs_evict_inode,
    .write_inode = snatchfs_write_inode,
};