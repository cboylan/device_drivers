#include <linux/module.h>
#include <linux/init.h>

#include "sstore.h"

static struct file_operations sstore_fops = {
    .owner   = THIS_MODULE,
    .open    = sstore_open,
    .release = sstore_release,
    .read    = sstore_read,
    .write   = sstore_write,
    .ioctl   = sstore_ioctl,
};

static dev_t sstore_dev_number;
struct class * sstore_class;

static int sstore_open(struct inode * inode, struct file * file){
}

static int sstore_release(struct inode * inode, struct file * file){
}

static size_t sstore_read(struct file * file, char __user * buf, size_t lbuf, loff_t * ppos){
}

static size_t sstore_write(struct file * file, const char __user * buf, size_t lbuf, loff_t * ppos){
}

static int sstore_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg){
}

static int __init sstore_init (void)
{
    printk(KERN_INFO "Hello: module loaded at 0x%p\n", sstore_init);
    return 0;
}

static void __exit sstore_exit (void)
{
    printk(KERN_INFO "Hello: module unloaded from 0x%p\n", sstore_exit);
}

module_init (sstore_init);
module_exit (sstore_exit);

MODULE_AUTHOR ("Clark Boylan");
MODULE_LICENSE ("GPL v2");
