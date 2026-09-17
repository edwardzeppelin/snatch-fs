#include <linux/module.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "include/snatchfs.h"
#include "include/fs_operations.h"

static struct file_system_type snatchfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "snatchfs",
    .mount = snatchfs_mount,
    .kill_sb = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
};

static int __init snatchfs_init(void)
{
    int ret = register_filesystem(&snatchfs_fs_type);
    if (ret) {
        printk(KERN_ERR "snatchFS: Failed to register filesystem\n");
        return ret;
    }
    printk(KERN_INFO "snatchFS: Module loaded successfully\n");
    return 0;
}

static void __exit snatchfs_exit(void)
{
    int ret = unregister_filesystem(&snatchfs_fs_type);
    if (ret)
        printk(KERN_ERR "snatchFS: Failed to unregister filesystem\n");
    printk(KERN_INFO "snatchFS: Module unloaded\n");
}

module_init(snatchfs_init);
module_exit(snatchfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You Know My Name");
MODULE_DESCRIPTION("snatch File System");