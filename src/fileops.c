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

// Return char data from given file
int datafetch(char *buffer, const char *pathname) {
/* TODO
 *  * Error handling
 *  * Malloc the buf
 */
    int filesize;
    if((filesize = procsize(pathname)) < 1)
        return -1;
    char *mal = (char *) malloc(filesize + 1);
    int fd = openat(AT_FDCWD, pathname, O_RDONLY);
    read(fd, mal, filesize);
    mal[filesize + 1] = '\0';
    close(fd);
    snprintf(buffer, filesize, "%s", strdup(mal));
    free(mal);
    return 0;
}

mode_t what_am_i(const char *path) {
    struct stat st;
    if((stat(path, &st)) == -1)
        perror("stat");

    return st.st_mode & S_IFMT;
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
    char filecontent[BUFSIZE];
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

const char *add_proc(const char *path) {

    char *final_path = (char *) malloc(strlen(proc) + strlen(path));
    strcpy(final_path, proc);
    strcat(final_path, path);
    return final_path;
}

int print_dir(const char *path, char *arr[]) {
    // refs: https://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1046380353&id=1044780608
    //      https://stackoverflow.com/questions/7631054/how-to-return-an-array-of-strings-in-c
/*
 * TODO
 *   * malloc arr: get amount of files in dir first then use that
 */
    DIR           *d;
    struct dirent *dir;
    d = opendir(path);
    int count = 0;
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            size_t len = strlen(dir->d_name);
            arr[count] = malloc(len + 1);
            if(!arr[count]) {
                free(arr[count]);
                return -1;
            }
            strncpy(arr[count], dir->d_name, len);
            count++;
        }

        closedir(d);
    }
    return 0;
}

int dir_size(const char *path) {

    DIR           *d;
    struct dirent *dir;
    d = opendir(path);
    int count = 0;
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            count++;
        }

        closedir(d);
    }
    return count;
}
