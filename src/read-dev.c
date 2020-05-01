//
// Created by george on 18/04/2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

const char* path = "/proc/net/dev";

int procsize(const char *pathname) {
    // All files in /proc are 0 in size, so must access the files and count chars manually

    char buf[1024];
    int count = 0;
    int fd = openat(AT_FDCWD, path, O_RDONLY);
    while(read(fd, buf, 1) > 0) {
        count++;
    }
    close(fd);
    return count;
}

char *datafetch(const char *pathname) {
    // Return char data from given file

    int filesize = procsize(pathname);
    char buf[filesize++];
    int fd = openat(AT_FDCWD, pathname, O_RDONLY);
    read(fd, buf, filesize);
    buf[filesize++] = '\0'; // for safety
    close(fd);
    return strdup(buf);
}

int main(int argc, char *argv[]) {

    char *buffer = datafetch(path);
    printf(buffer);
    return 0;
}
