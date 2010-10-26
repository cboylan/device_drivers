#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/wait.h>
#include <linux/capability.h>
#include <asm/uaccess.h>

#include "sstore_kernel.h"

static unsigned int num_of_blobs = 32;
static unsigned int blob_size = 128;
module_param(num_of_blobs, uint, S_IRUGO);
module_param(blob_size, uint, S_IRUGO);

static int sstore_open(struct inode *, struct file *);
static int sstore_release(struct inode *, struct file *);
static ssize_t sstore_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t sstore_write(struct file *, const char __user *, size_t, loff_t *);
static int sstore_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

static struct file_operations sstore_fops = {
    .owner   = THIS_MODULE,
    .open    = sstore_open,
    .release = sstore_release,
    .read    = sstore_read,
    .write   = sstore_write,
    .ioctl   = sstore_ioctl,
};

static dev_t sstore_dev_number;
static dev_t sstore_major = 0;
static dev_t sstore_minor = 0;
struct class * sstore_class;
struct sstore_dev * sstore_devp[NUM_SSTORE_DEVICES];

static int sstore_open(struct inode * inode, struct file * file){
    if(!capable(CAP_SYS_ADMIN)){
        return -EPERM;
    }

    printk(KERN_DEBUG "sstore: file opened.\n");
    /* Tie the sstore_dev struct for this "file" with its file * */
    file->private_data = container_of(inode->i_cdev, struct sstore_dev, cdev);

    return 0;
}

static int sstore_release(struct inode * inode, struct file * file){
    int i;
    struct sstore_dev * devp = file->private_data;

    printk(KERN_DEBUG "sstore: file released.\n");
    for(i = 0; i < num_of_blobs; ++i){
        if(devp->sstore_blobp[i]){
            if(devp->sstore_blobp[i]->data){
                kfree(devp->sstore_blobp[i]->data);
            }
            kfree(devp->sstore_blobp[i]);
        }
    }

    return 0;
}

static ssize_t sstore_read(struct file * file, char __user * buf, size_t lbuf, loff_t * ppos){
    int bytes_not_copied = 0;
    struct sstore_blob blob;
    struct sstore_dev * devp = file->private_data;

    if(lbuf != sizeof(struct sstore_blob)){
        return -EPERM;
    }

    bytes_not_copied = copy_from_user(&blob, buf, sizeof(struct sstore_blob));
    if(bytes_not_copied != 0){
        return -EIO;
    }

    if(blob.index >= num_of_blobs || blob.size > blob_size){
        return -EPERM;
    }

    mutex_lock(&devp->sstore_lock);
    while(devp->sstore_blobp[blob.index] == NULL){
        mutex_unlock(&devp->sstore_lock);
        if(wait_event_interruptible(devp->sstore_wq, devp->sstore_blobp[blob.index] != NULL)){
            return -ERESTARTSYS;
        }
        mutex_lock(&devp->sstore_lock);
    }
    bytes_not_copied = copy_to_user(blob.data, devp->sstore_blobp[blob.index]->data, blob.size);
    mutex_unlock(&devp->sstore_lock);

    printk(KERN_DEBUG "sstore: Successful read.\n");
    return blob.size - bytes_not_copied;
}

static ssize_t sstore_write(struct file * file, const char __user * buf, size_t lbuf, loff_t * ppos){
    int bytes_not_copied = 0;
    struct sstore_blob blob;
    struct sstore_blob * blobp;
    char * new_data;
    struct sstore_dev * devp = file->private_data;

    if(lbuf != sizeof(struct sstore_blob)){
        return -EPERM;
    }

    bytes_not_copied = copy_from_user(&blob, buf, sizeof(struct sstore_blob));
    if(bytes_not_copied != 0){
        return -EIO;
    }

    if(blob.index >= num_of_blobs || blob.size > blob_size){
        return -EPERM;
    }

    new_data = kmalloc(blob.size, GFP_KERNEL);
    if(!new_data){
        return -ENOMEM;
    }
    bytes_not_copied = copy_from_user(new_data, blob.data, blob.size);

    mutex_lock(&devp->sstore_lock);
    if(devp->sstore_blobp[blob.index] == NULL){
        mutex_unlock(&devp->sstore_lock);
        blobp = kmalloc(sizeof(struct sstore_blob), GFP_KERNEL);
        if(!blobp){
            kfree(new_data);
            return -ENOMEM;
        }
        mutex_lock(&devp->sstore_lock);
        devp->sstore_blobp[blob.index] = blobp;
    }
    else{
        mutex_unlock(&devp->sstore_lock);
        kfree(devp->sstore_blobp[blob.index]->data);
        mutex_lock(&devp->sstore_lock);
    }

    devp->sstore_blobp[blob.index]->size = blob.size;
    devp->sstore_blobp[blob.index]->index = blob.index;
    devp->sstore_blobp[blob.index]->data = new_data;
    mutex_unlock(&devp->sstore_lock);

    wake_up(&devp->sstore_wq);

    printk(KERN_DEBUG "sstore: Successful write.\n");
    return blob.size - bytes_not_copied;
}

static int sstore_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg){
}

static int __init sstore_init (void)
{
    /* TODO: Cleanup if error occurs in the middle of this function */
    int i;
    int ret;

    if(alloc_chrdev_region(&sstore_dev_number, sstore_minor, NUM_SSTORE_DEVICES, DEV_NAME) < 0){
        printk(KERN_DEBUG "sstore: unable to register device.\n");
        return -EPERM;
    }
    sstore_major = MAJOR(sstore_dev_number);

    sstore_class = class_create(THIS_MODULE, DEV_NAME);

    for(i = 0; i < NUM_SSTORE_DEVICES; ++i){
        /* Allocate memory for the per-device structure */
        sstore_devp[i] = kmalloc(sizeof(struct sstore_dev), GFP_KERNEL);
        if (!sstore_devp[i]){
            printk(KERN_DEBUG "sstore: bad kmalloc\n");
            return -ENOMEM;
        }

        sstore_devp[i]->sstore_number = i;

        sstore_devp[i]->sstore_blobp = kzalloc(num_of_blobs * sizeof(struct sstore_dev *), GFP_KERNEL);
        if (!sstore_devp[i]->sstore_blobp){
            printk(KERN_DEBUG "sstore: bad kzalloc\n");
            return -ENOMEM;
        }

        /* TODO: Init lock(s) */
        mutex_init(&sstore_devp[i]->sstore_lock);
        init_waitqueue_head(&sstore_devp[i]->sstore_wq);

        /* Connect the file operations with the cdev */
        cdev_init(&sstore_devp[i]->cdev, &sstore_fops);
        sstore_devp[i]->cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&sstore_devp[i]->cdev, MKDEV(sstore_major, sstore_minor + i), 1);
        if (ret) {
            printk(KERN_DEBUG "sstore: bad cdev\n");
            return ret;
        }

        /* Send uevents to udev, so it'll create /dev nodes */
        device_create(sstore_class, NULL, MKDEV(sstore_major, sstore_minor + i), "sstore%d", i);
    }

    printk(KERN_INFO "sstore: driver initialized.\n");
    return 0;
}

static void __exit sstore_exit (void)
{
    int i;

    /* Release the major number */
    unregister_chrdev_region(MKDEV(sstore_major, sstore_minor), NUM_SSTORE_DEVICES);

    /* Release I/O region */
    for (i = 0; i < NUM_SSTORE_DEVICES; ++i){
        device_destroy (sstore_class, MKDEV(sstore_major, sstore_minor + i));
        cdev_del(&sstore_devp[i]->cdev);
        kfree(sstore_devp[i]->sstore_blobp);
        kfree(sstore_devp[i]);
    }

    /* Destroy sstore_class */
    class_destroy(sstore_class);

    printk(KERN_INFO "sstore: driver removed.\n");
    return;
}

module_init (sstore_init);
module_exit (sstore_exit);

MODULE_AUTHOR ("Clark Boylan");
MODULE_LICENSE ("GPL v2");
