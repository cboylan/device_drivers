/* Clark Boylan
   CS 572
   Homework 1
   sstore driver
   sstore_kernel.h
   11/03/2010 */

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <asm/atomic.h>

#include "sstore_shared.h"

#define NUM_SSTORE_DEVICES 2
#define DEV_NAME "sstore"
#define STATS_NAME "stats"
#define DATA_NAME "data"

#define SSTORE_DELETE _IO(SSTORE_IOCTL_TYPE, 1)

struct sstore_dev {
    struct sstore_blob ** sstore_blobp;
    int sstore_number;
    struct cdev cdev;
    int open_count;
    int read_count;
    int write_count;
    int del_count;
    struct mutex sstore_lock;
    wait_queue_head_t sstore_wq;
};
