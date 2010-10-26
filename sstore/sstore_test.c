#include <stdlib.h>
#include <stdio.h>

#include "sstore_shared.h"

static const int number_of_args = 3;
static const char test_string[] = "Testing";

int main(int argv, char ** argc){
    int fd;
    int blob_size;
    int num_of_blobs;
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

    sstore_blob.size = strlen(test_string);
    sstore_blob.index = 0;
    sstore_blob.data = data;

    fd = open("/dev/sstore0", O_RDWR);
    write(fd, &blob, blob.size);
    strcpy(blob.data, "reset");
    read(fd, &blob, blob.size);
    close(fd);

    if(strcmp(blob.data, test_string) != 0){
        printf("Failure to read data out of kernel properly.\n");
    }

    return 0;
}
