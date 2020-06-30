#include "fileops.h"

int file_exists(const char *pathname) {
    int fd;
    if((fd = openat(AT_FDCWD, pathname, O_RDONLY)) < 0)
        // file doesn't exist
        return -1;
    return 0;
}

// All files in /proc are 0 in size, so must access the files and count chars manually
int procsize(const char *pathname) {

    char buf[BUFSIZE];
    int fd, count = 0;
    if((fd = openat(AT_FDCWD, pathname, O_RDONLY)) < 0)
        // file doesn't exist
        return -1;
    while(read(fd, buf, 1) > 0) {
        count++;
    }
    close(fd);
    return count;
}

int procsizefd(int fd) {

    char buf[BUFSIZE];
    int count = 0;
    if(fd < 0)
        // file doesn't exist
        return -1;
    while(read(fd, buf, 1) > 0) {
        count++;
    }
    return count;
}

// Return char data from given file
int datafetch(char *buffer, const char *pathname) {
/* TODO
 *  * Error handling
 *  * Malloc the buf
 */
    int filesize, fd;
    if((filesize = procsize(pathname)) < 1)
        return -1;
    if((fd = openat(AT_FDCWD, pathname, O_RDONLY)) == -1)
        return -1;
    read(fd, buffer, filesize);
    buffer[filesize + 1] = '\0';
    close(fd);
    return 0;
}

mode_t what_am_i(const char *path) {
    struct stat st;
    if((stat(path, &st)) == -1)
        perror("stat");

    return st.st_mode & S_IFMT;
}

// TODO: check the passthrough code to see if/how they use stat
int testing(const char *path, struct stat *st) {
    if((stat(path, st)) == -1) {
       perror("stat");
       return 1;
    }

    return 0;
}

struct stat *retstat(const char *path) {
    struct stat *tmpstbuf;
    if(stat(path, &tmpstbuf) == -1)
        printf("stat failed");
    return tmpstbuf;
}


// Given a filepath, fill the buf with its contents
size_t populate(char **buf, size_t size, off_t offset, const char *path) {
/* TODO
 */
    int len = procsize(path);
    char *filecontent = malloc(len + 1);
    if(datafetch(filecontent, path) < 0) {
        return -1;
    }
    if (offset >= len) {
        return 0;
    }
    if (offset + size > len) {
        memcpy(*buf, filecontent + offset, len - offset);
        return len - offset;
    }
    memcpy(*buf, filecontent + offset, size);
    return size;
}

const char *final_path(const char *path) {

    size_t len = strlen(proc) + strlen(path);
    char *final_path = malloc(len + 1);
    snprintf(final_path, len + 1, "%s%s", proc, path);
    return final_path;
}


char **dir_contents(const char *path) {
    // refs: https://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1046380353&id=1044780608
    //      https://stackoverflow.com/questions/7631054/how-to-return-an-array-of-strings-in-c

    struct dirent *dir;
    int i = 0, dirsize = dir_size(path);
    DIR *d = opendir(path);
    char **arr = malloc(sizeof(char *) * dirsize + 1);
    if(!arr) {
        free(arr);
        return NULL;
    }
    if(d) {
        while((dir = readdir(d))) {
            size_t len = strlen(dir->d_name);
            arr[i] = calloc(len + 1, sizeof(char));
            if(!arr[i]) {
//                printf("arr[%d] == NULL", i);
                free(arr[i]);
                return NULL;
            }
            snprintf(arr[i], len + 1, "%s", dir->d_name);
//            printf("arr[%d] = %s\n", i, arr[i]);
            i++;
        }
        closedir(d);
    } else {
        perror("opendir");
        return NULL;
    }
    return arr;
}

int dir_size(const char *path) {

    struct dirent *dir;
    DIR *d = opendir(path);
    int i = -1;
    if (d) {
        while ((dir = readdir(d))) {
            i++;
        }
        closedir(d);
    }
    return i;
}
