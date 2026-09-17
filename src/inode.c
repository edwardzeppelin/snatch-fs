#include <linux/buffer_head.h>
#include <linux/slab.h>
#include "snatchfs.h"

static struct kmem_cache *snatchfs_inode_cache;

struct inode *snatchfs_iget(struct super_block *sb, unsigned long ino)
{
    struct buffer_head *bh;
    struct snatchfs_inode *disk_inode;
    struct inode *inode;
    uint32_t inode_block, block_offset;

    inode = iget_locked(sb, ino);
    if (!inode)
        return ERR_PTR(-ENOMEM);
    if (!(inode->i_state & I_NEW))
        return inode;

    inode_block = (ino / snatchFS_INODES_PER_BLOCK) + ((struct snatchfs_superblock *)sb->s_fs_info)->first_inode;
    block_offset = ino % snatchFS_INODES_PER_BLOCK;

    bh = sb_bread(sb, inode_block);
    if (!bh) {
        iget_failed(inode);
        return ERR_PTR(-EIO);
    }

    disk_inode = (struct snatchfs_inode *)bh->b_data + block_offset;

    inode->i_mode = disk_inode->mode;
    inode->i_uid = disk_inode->uid;
    inode->i_gid = disk_inode->gid;
    inode->i_size = disk_inode->size;
    inode->i_atime.tv_sec = disk_inode->atime;
    inode->i_mtime.tv_sec = disk_inode->mtime;
    inode->i_ctime.tv_sec = disk_inode->ctime;
    inode->i_ino = ino;

    if (S_ISDIR(inode->i_mode)) {
        inode->i_op = &snatchfs_dir_inode_operations;
        inode->i_fop = &snatchfs_dir_operations;
    } else if (S_ISREG(inode->i_mode)) {
        inode->i_op = &snatchfs_file_inode_operations;
        inode->i_fop = &snatchfs_file_operations;
        inode->i_mapping->a_ops = &snatchfs_aops;
    } else if (S_ISLNK(inode->i_mode)) {
        inode->i_op = &snatchfs_symlink_inode_operations;
        inode_nohighmem(inode);
    }

    brelse(bh);
    unlock_new_inode(inode);
    return inode;
}

static void snatchfs_free_inode(struct inode *inode)
{
    kmem_cache_free(snatchfs_inode_cache, snatchFS_I(inode));
}

static void init_once(void *foo)
{
    struct snatchfs_inode_info *ei = foo;
    inode_init_once(&ei->vfs_inode);
}

int __init snatchfs_init_inodecache(void)
{
    snatchfs_inode_cache = kmem_cache_create("snatchfs_inode_cache",
                                         sizeof(struct snatchfs_inode_info),
                                         0, SLAB_RECLAIM_ACCOUNT,
                                         init_once);
    return snatchfs_inode_cache ? 0 : -ENOMEM;
}

void snatchfs_destroy_inodecache(void)
{
    kmem_cache_destroy(snatchfs_inode_cache);
}