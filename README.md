# snatchFS — Custom Linux Kernel Filesystem

`snatchFS` is a lightweight, ext2-inspired block file system written in C for the Linux kernel. It includes a custom Kernel Module (LKM) integrating with the Linux VFS (Virtual File System) and standalone userspace tools for formatting (`mkfs`) and consistency checking (`fsck`).

## Features

- **Fixed 4KB Block Architecture**: Simplifies allocation and sector alignment.
- **Linux VFS Integration**: Full support for standard POSIX file operations (`read`, `write`, `mmap`, `llseek`).
- **Directory & Symlink Management**: Custom inode operations for directories and symbolic links.
- **Page Cache Support**: Built using `address_space_operations` (`mpage_readpage`, `block_write_full_page`).
- **Userspace Toolchain**: Custom `mkfs.snatchfs` for low-level formatting and `fsck.snatchfs` for disk verification.

## Disk Layout

+----------------+------------------+------------------+-----------------+-------------------+
| Block 0        | Block 1          | Block 2          | Block 3         | Block 4+          |
| Superblock     | Block Bitmap     | Inode Bitmap     | Inode Table     | Data Blocks       |
| (Magic 0xDEAD) |                  |                  |                 | (Root dir at #4)  |
+----------------+------------------+------------------+-----------------+-------------------+

## Technical Specifications

| Parameter | Value |
| :--- | :--- |
| **Magic Number** | `0xDEADBEEF` |
| **Block Size** | 4096 bytes |
| **Direct Block Pointers** | 12 |
| **Indirect Block Pointers** | 1 |
| **Max Filename Length** | 255 bytes |

## Building and Installation

### Prerequisites

Ensure you have kernel headers and standard build tools installed:

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

Build the Kernel Module & Tools

```bash
make all      # Builds snatchfs.ko kernel module
make format   # Compiles mkfs.snatchfs in tools/
```

Usage Workflow
1. Create a loopback device or virtual image:

```bash
dd if=/dev/zero of=disk.img bs=1M count=64
```

2. Format the image with snatchFS:

```bash
./tools/mkfs.snatchfs disk.img 64
```

3. Load the kernel module:

```bash
sudo insmod snatchfs.ko
```

4. Mount the filesystem:

```bash
mkdir -p /mnt/snatch
sudo mount -o loop -t snatchfs disk.img /mnt/snatch
```

5. Unmount and unload:

```bash
sudo umount /mnt/snatch
sudo rmmod snatchfs
```

6. Check filesystem integrity:

```bash
./tools/fsck.snatchfs disk.img
```
