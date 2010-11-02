/*
   Test "suite" for the sstore driver.
   Test Plan:
   Fork
   Parent writes to index. Write contains next index.
   Child reads from index. Read next index.
   Parent reads from next index.
   Child writes to next index.
   Child exits.
   Parent uses ictl to delete two indices.
   Parent exits.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stropts.h>
#include <fcntl.h>
#include <sys/time.h>

#include "sstore_shared.h"

#define SSTORE_IOCTL (1 | (SSTORE_IOCTL_TYPE << 8))

static const int number_of_args = 2;
static int blob_size;
static int num_of_blobs;

static const char test_string[] = "Testing";
static char * dev_file;
static const char blob_size_loc[] = "/sys/module/sstore/parameters/blob_size";
static const char num_blobs_loc[] = "/sys/module/sstore/parameters/num_of_blobs";

static void child_proc();
static void parent_proc();
static int read_int(const char * file_name);
static int Open(const char * file_name, int flags);
static int Close(int fd);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);

int main(int argv, char ** argc){
    pid_t child_pid;

    if(argv < number_of_args){
        printf("Wrong number of arguments. Need %d arguments.\n", number_of_args);
        return 1;
    }
    dev_file = argc[1];

    num_of_blobs = read_int(num_blobs_loc);
    blob_size = read_int(blob_size_loc);

    child_pid = fork();
    if(child_pid == 0){
        child_proc();
    }
    else{
        parent_proc();
    }

    return 0;
}

static void child_proc(){
    int fd;
    int count;
    int read_index;
    int rand_index;
    struct timeval tv;
    struct sstore_blob blob;

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    blob.size = sizeof(int);
    blob.index = 0;
    blob.data = (char *) &read_index;

    while(1){
        fd = Open(dev_file, O_RDWR);

        count = Read(fd, &blob, sizeof(blob));
        if(count != sizeof(int)){
            printf("Child: only %d bytes read.\n", count);
        }
        if(ioctl(fd, SSTORE_IOCTL, blob.index) != 0){
            printf("IOCTL delete on index %d failed.\n", blob.index);
        }

        printf("Child: next index %d.\n", read_index);

        blob.size = sizeof(int);
        blob.index = read_index;
        while((rand_index = rand() % num_of_blobs) == read_index);
        blob.data = (char *) &rand_index;

        printf("Child: writing %d to index %d.\n", rand_index, blob.index);
        count = Write(fd, &blob, sizeof(blob));
        if(count != sizeof(int)){
            printf("Child: only %d bytes written.\n", count);
        }

        Close(fd);

        blob.size = sizeof(int);
        blob.index = rand_index;
        blob.data = (char *) &read_index;
    }
}

static void parent_proc(){
    int fd;
    int count;
    int read_index;
    int rand_index;
    int status;
    struct timeval tv;
    struct sstore_blob blob;

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    blob.size = sizeof(int);
    blob.index = 0;
    while((rand_index = rand() % num_of_blobs) == read_index);
    blob.data = (char *) &rand_index;

    while(1){
        fd = Open(dev_file, O_RDWR);

        printf("Parent: writing %d to index %d.\n", rand_index, blob.index);
        count = Write(fd, &blob, sizeof(blob));
        if(count != sizeof(int)){
            printf("Parent: only %d bytes written.\n", count);
        }

        blob.size = sizeof(int);
        blob.index = rand_index;
        blob.data = (char *) &read_index;

        count = Read(fd, &blob, sizeof(blob));
        if(count != sizeof(int)){
            printf("Parent: only %d bytes read.\n", count);
        }
        if(ioctl(fd, SSTORE_IOCTL, blob.index) != 0){
            printf("IOCTL delete on index %d failed.\n", blob.index);
        }

        printf("Parent: next index %d.\n", read_index);

        Close(fd);

        blob.size = sizeof(int);
        blob.index = read_index;
        while((rand_index = rand() % num_of_blobs) == read_index);
        blob.data = (char *) &rand_index;
    }

    wait(&status);
}

static int read_int(const char * file_name){
    int data;

    FILE * fp = fopen(num_blobs_loc, "r");
    if(!fp){
        printf("Unable to read from %s.\n", file_name);
        return 1;
    } 
    if(fscanf(fp, "%d\n", &data) != 1){
        printf("Unable to read from %s.\n", file_name);
        return 1;
    }
    fclose(fp);

    return data;
}

static int Open(const char * file_name, int flags){
    int fd;

    fd = open(file_name, flags);
    if(fd == -1){
        printf("Error opening %s.\n", file_name);
    }

    return fd;
}

static int Close(int fd){
    int ret;

    ret = close(fd);
    if(ret != 0){
        printf("close() failed.\n");
    }

    return ret;
}

ssize_t Read(int fd, void *buf, size_t count){
    ssize_t byte_count;

    byte_count = read(fd, buf, count);
    if(byte_count == -1){
        perror("Read failed");
    }

    return byte_count;
}

ssize_t Write(int fd, const void *buf, size_t count){
    ssize_t byte_count;

    byte_count = write(fd, buf, count);
    if(byte_count == -1){
        perror("Write failed");
    }

    return byte_count;
}
