#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "sstore_shared.h"

static const int number_of_args = 3;
static const char test_string[] = "Testing";

int main(int argv, char ** argc){
    int fd;
    int blob_size;
    int num_of_blobs;
    int byte_count;
    char * data;
    struct sstore_blob blob;

    if(argv < number_of_args){
        printf("Wrong number of arguments. Need %d arguments.\n", number_of_args);
        return 1;
    }
    num_of_blobs = atoi(argc[1]);
    blob_size = atoi(argc[2]);

    data = (char *) malloc(blob_size);
    strcpy(data, test_string);

    blob.size = strlen(test_string);
    blob.index = 0;
    blob.data = data;

    fd = open("/dev/sstore0", O_RDWR);
    if(fd == -1){
        printf("Error opening dev file.\n");
    }

    byte_count = write(fd, &blob, blob.size);
    if(byte_count != blob.size){
        printf("Only %d bytes written.\n", byte_count);
    }

    strcpy(blob.data, "reset");

    byte_count = read(fd, &blob, blob.size);
    if(byte_count != blob.size){
        printf("Only %d bytes read.\n", byte_count);
    }

    if(close(fd) != 0){
        printf("close() failed.\n");
    }

    if(strcmp(blob.data, test_string) != 0){
        printf("Failure to read data out of kernel properly.\n");
    }

    return 0;
}
