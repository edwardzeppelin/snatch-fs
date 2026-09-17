obj-m += snatchfs.o
snatchfs-objs := src/super.o src/inode.o src/bitmap.o src/file.o src/dir.o src/module.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
    $(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
    $(MAKE) -C $(KDIR) M=$(PWD) clean

install:
    sudo insmod snatchfs.ko

uninstall:
    sudo rmmod snatchfs

format:
    gcc -o tools/mkfs.snatchfs tools/mkfs.c -Iinclude