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
#include <fcntl.h>

#include "sstore_shared.h"

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

    pid_t = fork();
    if(pid_t == 0){
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
    int next_index;
    struct sstore_blob blob;

    blob.size = sizeof(int);
    blob.index = 0;
    blob.data = &next_index;

    fd = Open(dev_file, O_RDWR);

    count = Read(fd, &blob, sizeof(blob));
    if(count != sizeof(int)){
        printf("Child: only %d bytes read.\n", count);
    }

    printf("Child: next index %d.\n", next_index);

    blob.size = sizeof(int);
    blob.index = next_index;
    next_index++;
    blob.data = &next_index;

    count = Write(fd, &blob, sizeof(blob));
    if(count != sizeof(int)){
        printf("Child: only %d bytes written.\n", count);
    }
}

static void parent_proc(){
    int fd;
    int count;
    int next_index;
    int status;
    struct sstore_blob blob;

    blob.size = sizeof(int);
    blob.index = 0;
    next_index = blob.index + 1;
    blob.data = &next_index;

    fd = Open(dev_file, O_RDWR);

    count = Write(fd, &blob, sizeof(blob));
    if(count != sizeof(int)){
        printf("Parent: only %d bytes written.\n", count);
    }

    blob.size = sizeof(int);
    blob.index = next_index;
    blob.data = &next_index;

    count = Read(fd, &blob, sizeof(blob));
    if(count != sizeof(int)){
        printf("Child: only %d bytes read.\n", count);
    }
    printf("Parent: next index %d.\n", next_index);

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
