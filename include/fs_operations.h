#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H

#include <linux/fs.h>

struct dentry *snatchfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);

struct inode *snatchfs_iget(struct super_block *sb, unsigned long ino);
int snatchfs_write_inode(struct inode *inode, struct writeback_control *wbc);
void snatchfs_evict_inode(struct inode *inode);

int snatchfs_file_mmap(struct file *file, struct vm_area_struct *vma);
ssize_t snatchfs_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);
ssize_t snatchfs_file_write_iter(struct kiocb *iocb, struct iov_iter *iter);

int snatchfs_readdir(struct file *file, struct dir_context *ctx);
int snatchfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);
int snatchfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);
int snatchfs_rmdir(struct inode *dir, struct dentry *dentry);
int snatchfs_unlink(struct inode *dir, struct dentry *dentry);

const char *snatchfs_get_link(struct dentry *dentry, struct inode *inode, struct delayed_call *done);
int snatchfs_symlink(struct inode *dir, struct dentry *dentry, const char *symname);

#endif // FS_OPERATIONS_H