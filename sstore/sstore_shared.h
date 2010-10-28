#define SSTORE_IOCTL_TYPE 'k'

struct sstore_blob {
    int size;
    int index;
    char * data;
};
