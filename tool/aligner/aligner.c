#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    struct stat file_stat;
    int ret;
    off_t file_size;
    off_t remain_size;
    off_t cur_offset;
    int align_size;
    int filefd;
    char buf = 0x90; /* nop */

    if (argc != 3) {
        fprintf(stderr, 
                "Argument is not valid\n"
                "Usage) %s <filename> <align size>\n", argv[0]);
        return 1;
    }

    align_size = atoi(argv[2]);
    if (align_size < 1) {
        fprintf(stderr, "Invalid align size\n");
        return 2;
    }

    ret = stat(argv[1], &file_stat);
    if (ret == -1) {
        fprintf(stderr, "Fail to get file stat(%d:%s)\n",
                errno, strerror(errno));
        return 3;
    }
    
    printf("Original file size is %d bytes\n", file_stat.st_size);
    remain_size = align_size - file_stat.st_size % align_size;
    if (remain_size == 0) {
        printf("Nothing to do, because file is already aligned\n");
        return 0;
    }

    filefd = open(argv[1], O_APPEND | O_WRONLY);
    if (filefd < 0) {
        fprintf(stderr, "Fail to open the file\n");
        return 4;
    }
    printf("Open the file\n");

    cur_offset = lseek(filefd, 0, SEEK_END);
    if (cur_offset == -1) {
        fprintf(stderr, "Fail to change position(%d:%s)\n",
                errno, strerror(errno));
        return 5;
    }
    printf("Move position to end of file(%d bytes)\n", cur_offset);

    printf("Write %d dummy bytes\n", remain_size);
    while (remain_size--) {
        write(filefd, &buf, 1);
    }

    close(filefd);

    printf("Success\n");

    return 0;
}
