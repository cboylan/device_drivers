#include <linux/list.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define NUM_SSTORE_DEVICES 2
#define NAME_SIZE 8
#define DEV_NAME "sstore"

struct sstore_dev {
    struct list_head sstore_head;
    unsigned int blob_size;
    int sstore_number;
    struct cdev cdev;
    char sstore_name[NAME_SIZE];
};

static int sstore_open(struct inode *, struct file *);
static int sstore_release(struct inode *, struct file *);
static ssize_t sstore_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t sstore_write(struct file *, const char __user *, size_t, loff_t *);
static int sstore_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
