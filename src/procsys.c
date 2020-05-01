#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* TODO
 *  * Create a directory for /net
 *  * Put the dev file in /net
 *  *
*/

static const char *filepath = "/dev";
static const char *filename = "dev";
static const char *pndpath = "/proc/net/dev";

int procsize(const char *pathname) {
    // All files in /proc are 0 in size, so must access the files and count chars manually

    char buf[1024];
    int count = 0;
    int fd = openat(AT_FDCWD, pathname, O_RDONLY);
    while(read(fd, buf, 1) > 0) {
        count++;
    }
    close(fd);
    return count;
}

char *datafetch(const char *pathname) {
    // Return char data from given file

    int filesize = procsize(pathname);
    char buf[filesize+1];
    int fd = openat(AT_FDCWD, pathname, O_RDONLY);
    read(fd, buf, filesize);
    buf[filesize+1] = '\0'; // for safety
    close(fd);
    return strdup(buf);
}

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (strcmp(path, filepath) == 0) {
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
//  filler(buf, "net/", NULL, 0);

  filler(buf, filename, NULL, 0);

  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

  if (strcmp(path, filepath) == 0) {
      int len = procsize(pndpath);
      char *filecontent = datafetch(pndpath);
    if (offset >= len) {
      return 0;
    }

    if (offset + size > len) {
      memcpy(buf, filecontent + offset, len - offset);
      return len - offset;
    }

    memcpy(buf, filecontent + offset, size);
    return size;
  }

  return -ENOENT;
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
