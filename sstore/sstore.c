#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/wait.h>
#include <linux/capability.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>
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
static long sstore_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations sstore_fops = {
    .owner          = THIS_MODULE,
    .open           = sstore_open,
    .release        = sstore_release,
    .read           = sstore_read,
    .write          = sstore_write,
    .unlocked_ioctl = sstore_ioctl,
};

static dev_t sstore_dev_number;
static dev_t sstore_major = 0;
static dev_t sstore_minor = 0;
static struct class * sstore_class;
static struct sstore_dev * sstore_devp[NUM_SSTORE_DEVICES];
static struct proc_dir_entry * proc_dir = NULL;

static int sstore_open(struct inode * inode, struct file * file)
{
    struct sstore_dev * devp;

    if(!capable(CAP_SYS_ADMIN)){
        return -EPERM;
    }

    /* Tie the sstore_dev struct for this "file" with its file * */
    file->private_data = container_of(inode->i_cdev, struct sstore_dev, cdev);

    devp = file->private_data;
    mutex_lock(&devp->sstore_lock);
    devp->open_count++;
    mutex_unlock(&devp->sstore_lock);

    printk(KERN_DEBUG "sstore: file opened.\n");

    return 0;
}

static int sstore_release(struct inode * inode, struct file * file)
{
    int i;
    struct sstore_dev * devp = file->private_data;

    mutex_lock(&devp->sstore_lock);
    devp->open_count--;
    if(devp->open_count == 0){
        for(i = 0; i < num_of_blobs; ++i){
            if(devp->sstore_blobp[i]){
                if(devp->sstore_blobp[i]->data){
                    kfree(devp->sstore_blobp[i]->data);
                }
                kfree(devp->sstore_blobp[i]);
                devp->sstore_blobp[i] = NULL;
            }
        }
    }
    mutex_unlock(&devp->sstore_lock);

    printk(KERN_DEBUG "sstore: file released.\n");

    return 0;
}

static ssize_t sstore_read(struct file * file, char __user * buf, size_t lbuf, loff_t * ppos)
{
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

static ssize_t sstore_write(struct file * file, const char __user * buf, size_t lbuf, loff_t * ppos)
{
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

static long sstore_ioctl(struct file * file, unsigned int cmd, unsigned long arg)
{
    struct sstore_dev * devp = file->private_data;
    if(cmd == SSTORE_DELETE){
        mutex_lock(&devp->sstore_lock);
        if(devp->sstore_blobp[arg]){
            if(devp->sstore_blobp[arg]->data){
                kfree(devp->sstore_blobp[arg]->data);
            }
            kfree(devp->sstore_blobp[arg]);
            devp->sstore_blobp[arg] = NULL;
            mutex_unlock(&devp->sstore_lock);
        }
        else{
            mutex_unlock(&devp->sstore_lock);
            return -EINVAL;
        }
    }
    else{
        return -EINVAL;
    }

    return 0;
}

void * sstore_seq_start(struct seq_file *m, loff_t *pos)
{
}

void * sstore_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
}

void * sstore_seq_stop(struct seq_file *m, void *v)
{
}

int sstore_seq_show(struct seq_file *m, void *v)
{
    return 0;
}

static struct seq_operations sstore_seq_ops = {
    .start = sstore_seq_start,
    .next = sstore_seq_next,
    .stop = sstore_seq_stop,
    .show = sstore_seq_show,
}

static  int sstore_seq_open(struct inode * inode, struct file * file)
{
    return seq_open(file, &sstore_seq_ops);
}

static struct file_operations sstore_proc_data_fops = {
    .owner = THIS_MODULE,
    .open = sstore_seq_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

int sstore_stats_read_proc(char * page, char **start, off_t off, int count, int *eof, void *data)
{
    return 0;
}

static int sstore_proc_init(void)
{
    int rv = 0;
    struct proc_dir_entry * sstore_proc_stats = NULL;
    struct proc_dir_entry * sstore_proc_data = NULL;

    sstore_proc_dir = proc_mkdir(DEV_NAME, NULL);
    if(sstore_proc_dir == NULL){
        rv = -ENOMEM;
        goto out;
    }

    sstore_proc_stats = create_proc_read_entry(STATS_NAME, S_IRUSER, sstore_proc_dir, sstore_stats_read_proc, NULL);
    if(sstore_proc_stats == NULL){
        rv = -ENOMEM;
        goto no_stats;
    }
    sstore_proc_stats->owner = THIS_MODULE;

    sstore_proc_data = create_proc_entry(DATA_NAME, S_IRUSER, sstore_proc_dir);
    if(sstore_proc_data == NULL){
        rv = -ENOMEM;
        goto no_data;
    }
    sstore_proc_data->owner = THIS_MODULE;
    sstore_proc_data->proc_fops = &sstore_proc_data_fops;

    return 0;

no_data:
    remove_proc_entry(STATS_NAME, sstore_proc_dir);
no_stats:
    remove_proc_entry(DEV_NAME, NULL);
out:
    return rv;
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

        sstore_devp[i]->open_count = 0;
        sstore_devp[i]->sstore_number = i;

        sstore_devp[i]->sstore_blobp = kzalloc(num_of_blobs * sizeof(struct sstore_dev *), GFP_KERNEL);
        if (!sstore_devp[i]->sstore_blobp){
            printk(KERN_DEBUG "sstore: bad kzalloc\n");
            return -ENOMEM;
        }

        /* Init lock(s) */
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
    
    sstore_proc_init();

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

    /* Remove procfs entries */
    remove_proc_entry(STATS_NAME, sstore_proc_dir);
    remove_proc_entry(DATA_NAME, sstore_proc_dir);
    remove_proc_entry(DEV_NAME, NULL);

    printk(KERN_INFO "sstore: driver removed.\n");
    return;
}

module_init (sstore_init);
module_exit (sstore_exit);

MODULE_AUTHOR ("Clark Boylan");
MODULE_LICENSE ("GPL v2");
