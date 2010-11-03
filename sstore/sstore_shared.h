/* Clark Boylan
   CS 572
   Homework 1
   sstore driver
   sstore_shared.h
   11/03/2010 */

/* The 'k' type appears to be available. */
#define SSTORE_IOCTL_TYPE 'k'

struct sstore_blob {
    int size;
    int index;
    char * data;
};
