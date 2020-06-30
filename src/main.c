#define FUSE_USE_VERSION 30

#include "fileops.h"
#include <fuse3/fuse.h>

/*
 * TODO
 *   Revert back to working prog and try with fuse3 libs. Add passthrough logic modularly
 */
static int getattr_callback(const char *path, struct stat *stbuf) {
/* TODO
 *   * Check out the other struct stat elements and find out how to use them:
 *                    https://linux.die.net/man/2/stat
 *   * Also the fuse.h file gives more detail on each function:
 *                    https://github.com/libfuse/libfuse/blob/master/include/fuse.h
 */
    memset(stbuf, 0, sizeof(struct stat));
    const char *final_path = add_proc(path);
    if(lstat(final_path, stbuf) == -1) {
        return -ENOENT;
    }
    return 0;

//    switch (what_am_i(final_path)) {
//        case S_IFDIR:
//            stbuf->st_mode = S_IFDIR | 0755;
//            stbuf->st_nlink = 2; // Needed to work
//            return 0;
//        case S_IFREG:
//            stbuf->st_mode = S_IFREG | 0777;
////            stbuf->st_nlink = 1;
//            stbuf->st_size = procsize(final_path);
//            return 0;
//    }
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
/*
 * TODO
 *   * Why does it take 6s to ls root? Check for alternate filler commands
 */
    (void) offset; // To stop compiler warnings on unused variables
    (void) fi; // fi contains a file handle to be used in create, open and opendir

    const char *final_path = add_proc(path);
    int dir_f_size =  dir_size(final_path);
    char **dir_cnt = dir_contents(final_path);

    for (int i = 0; i < dir_f_size; i++) {
        filler(buf, dir_cnt[i], NULL, 0);
    }
    free(dir_cnt);
    return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    const char *final_path = add_proc(path);
    size_t final_size;
    if((final_size = populate(&buf, size, offset, final_path)) < 0)
        return -ENOENT;
    return final_size;
}

static int readlink_callback(const char *path, char *buf, size_t size)
{
    int res;
    const char *final_path = add_proc(path);

    res = readlink(final_path, buf, size - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}

static struct fuse_operations procsys_ops = {
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .readdir = readdir_callback,
    .readlink = readlink_callback,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &procsys_ops, NULL);
}
