#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "sstore_shared.h"

static const int number_of_args = 2;
static const char test_string[] = "Testing";
static const char blob_size_loc[] = "/sys/module/sstore/parameters/blob_size";
static const char num_blobs_loc[] = "/sys/module/sstore/parameters/num_of_blobs";

static int read_int(file_name);
static int Open(const char * file_name, int flags);
static int Close(int fd);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);

int main(int argv, char ** argc){
    int fd;
    FILE * fp;
    int blob_size;
    int num_of_blobs;
    int byte_count;
    char * file_name;
    char * data;
    struct sstore_blob blob;

    if(argv < number_of_args){
        printf("Wrong number of arguments. Need %d arguments.\n", number_of_args);
        return 1;
    }
    file_name = argc[1];
    num_of_blobs = read_int(num_blobs_loc);
    blob_size = read_int(blob_size_loc);

    data = (char *) malloc(blob_size);
    strcpy(data, test_string);

    blob.size = strlen(test_string);
    blob.index = 0;
    blob.data = data;

    fd = Open(file_name, O_RDWR);

    byte_count = Write(fd, &blob, sizeof(struct sstore_blob));
    if(byte_count != blob.size){
        printf("Only %d bytes written.\n", byte_count);
    }

    strcpy(blob.data, "reset");

    byte_count = Read(fd, &blob, sizeof(struct sstore_blob));
    if(byte_count != blob.size){
        printf("Only %d bytes read.\n", byte_count);
    }

    Close(fd);

    if(strcmp(blob.data, test_string) != 0){
        printf("Failure to read data out of kernel properly.\n");
    }

    return 0;
}

static int read_int(file_name){
    int data;

    FILE * fp = fopen(num_blobs_loc, "r");
    if(!fp){
        printf("Unable to read from %s.\n", file_name);
        return 1;
    } 
    if(fscanf(fp, "%d\n", data) != 1){
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
