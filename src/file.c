#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include "snatchfs.h"

static int snatchfs_readpage(struct file *file, struct page *page)
{
    return mpage_readpage(page, snatchfs_get_block);
}

static int snatchfs_writepage(struct page *page, struct writeback_control *wbc)
{
    return block_write_full_page(page, snatchfs_get_block, wbc);
}

static int snatchfs_write_begin(struct file *file, struct address_space *mapping, loff_t pos, unsigned len, unsigned flags, struct page **pagep, void **fsdata)
{
    return block_write_begin(mapping, pos, len, flags, pagep, snatchfs_get_block);
}

static sector_t snatchfs_bmap(struct address_space *mapping, sector_t block)
{
    return generic_block_bmap(mapping, block, snatchfs_get_block);
}

const struct address_space_operations snatchfs_aops = {
    .readpage = snatchfs_readpage,
    .writepage = snatchfs_writepage,
    .write_begin = snatchfs_write_begin,
    .write_end = generic_write_end,
    .bmap = snatchfs_bmap,
};

const struct file_operations snatchfs_file_operations = {
    .llseek = generic_file_llseek,
    .read_iter = generic_file_read_iter,
    .write_iter = generic_file_write_iter,
    .mmap = generic_file_mmap,
    .fsync = generic_file_fsync,
    .splice_read = generic_file_splice_read,
    .splice_write = iter_file_splice_write,
};

const struct inode_operations snatchfs_file_inode_operations = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};