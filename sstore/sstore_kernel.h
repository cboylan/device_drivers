#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <asm/atomic.h>

#include "sstore_shared.h"

#define NUM_SSTORE_DEVICES 2
#define NAME_SIZE 8
#define DEV_NAME "sstore"

struct sstore_dev {
    struct sstore_blob ** sstore_blobp;
    int sstore_number;
    struct cdev cdev;
    int open_count;
    struct mutex sstore_lock;
    wait_queue_head_t sstore_wq;
    char sstore_name[NAME_SIZE];
};
