#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#define BUFSIZE 1024

/* TODO
 *  * Find ds to store dirs and files
 *  * Use ds to populate in for loop
 *  * Make does file exist function to make it more modular
*/

static const char *pndpath = "/proc/net/dev";
static char proc[] = "/proc";

// All files in /proc are 0 in size, so must access the files and count chars manually
int procsize(const char *pathname) {

    char buf[BUFSIZE];
    int count = 0;
    int fd = openat(AT_FDCWD, pathname, O_RDONLY);
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
    int filesize = procsize(pathname);
    if(filesize  < 1)
        // Doesn't exist (?)
        return -1;
    char buf[filesize+1]; // Should malloc this
    int fd = openat(AT_FDCWD, pathname, O_RDONLY);
    // can we just read it into buffer instead? Safe?
    read(fd, buf, filesize);
    buf[filesize+1] = '\0'; // for safety
    close(fd);
    char *tmp = strdup(buf);
    snprintf(buffer, filesize, "%s", tmp);
    return 0;
}

// Given a filepath, fill the buf with its contents
size_t populate(char **buf, size_t size, off_t offset, const char *path) {
/* TODO
 *  * return -EONENT upon fail
 */
    int len = procsize(path);
    char filecontent[BUFSIZE];
    if(datafetch(filecontent, path) < 0) {
        return -ENOENT; // is this smart?
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

static int getattr_callback(const char *path, struct stat *stbuf) {
/* TODO
 *  * For the getattr_callback:
 *    - Make function to check if path is referring to dir or file, and populate stbuf accordingly, '/' and '/net'
 *      should get the same attributes (b/c they are both dirs), while the files should get another. Could do this
 *      in the function itself with a switch or similar
 */

  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }
    if (strcmp(path, "/net") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

  if (strcmp(path, "/net/dev") == 0) {
      int len = procsize(pndpath);
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = len;
    return 0;
  }

  return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    if(strcmp(path, "/") == 0) {

      filler(buf, "net", NULL, 0);
  }
    if(strcmp(path, "/net") == 0) {
      filler(buf, "dev", NULL, 0);
    }

  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

    char *final_path = strncat(proc, path, strlen(path));
    return populate(&buf, size, offset, final_path);
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
};

int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
