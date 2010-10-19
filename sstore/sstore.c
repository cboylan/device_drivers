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
struct sstore_dev * sstore_devp[NUM_SSTORE_DEVICES];

static int sstore_open(struct inode * inode, struct file * file){
}

static int sstore_release(struct inode * inode, struct file * file){
}

static ssize_t sstore_read(struct file * file, char __user * buf, size_t lbuf, loff_t * ppos){
}

static ssize_t sstore_write(struct file * file, const char __user * buf, size_t lbuf, loff_t * ppos){
}

static int sstore_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg){
}

static int __init sstore_init (void)
{
    int i;
    int ret;

    if(alloc_chrdev_region(&sstore_dev_number, 0, NUM_SSTORE_DEVICES, DEV_NAME) < 0){
        printk(KERN_DEBUG "sstore: unable to register device.\n");
        return -EPERM;
    }

    //sstore_class = class_create(THIS_MODULE, DEV_NAME);

    for(i = 0; i < NUM_SSTORE_DEVICES; ++i){
        /* Allocate memory for the per-device structure */
        sstore_devp[i] = kmalloc(sizeof(struct sstore_dev), GFP_KERNEL);
        if (!sstore_devp[i]){
            printk(KERN_DEBUG "sstore: bad kmalloc\n");
            return -ENOMEM;
        }

        sstore_devp[i]->sstore_number = i;

        /* Connect the file operations with the cdev */
        cdev_init(&sstore_devp[i]->cdev, &sstore_fops);
        sstore_devp[i]->cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&sstore_devp[i]->cdev, (sstore_dev_number + i), 1);
        if (ret) {
            printk(KERN_DEBUG "sstore: bad cdev\n");
            return ret;
        }

        /* Send uevents to udev, so it'll create /dev nodes */
        device_create(sstore_class, NULL, MKDEV(MAJOR(sstore_dev_number), i), "sstore%d", i);
    }

    printk(KERN_INFO "sstore: driver initialized.\n");
    return 0;
}

static void __exit sstore_exit (void)
{
    int i;

    /* Release the major number */
    unregister_chrdev_region((sstore_dev_number), NUM_CMOS_BANKS);

    /* Release I/O region */
    for (i = 0; i<NUM_SSTORE_DEVICES; ++i){
        device_destroy (sstore_class, MKDEV(MAJOR(sstore_dev_number), i));
        cdev_del(&sstore_devp[i]->cdev);
        kfree(sstore_devp[i]);
    }

    /* Destroy sstore_class */
    //class_destroy(sstore_class);

    printk(KERN_INFO "sstore: driver removed.\n");
    return;
}

module_init (sstore_init);
module_exit (sstore_exit);

MODULE_AUTHOR ("Clark Boylan");
MODULE_LICENSE ("GPL v2");
