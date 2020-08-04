/*
 * Main skeleton taken from the libfuse example: passthrough.c
 */
#define FUSE_USE_VERSION 35

#ifdef HAVE_CONFIG_H
#include <fuse3/config.h>
#endif

#define _GNU_SOURCE

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "fileops.h"
#include "client-server.h"
#include "client.h"
#include "writer.h"
#include "defs.h"

int server_sock;
int CLIENT_FLAG;
int pipecomms[2];

/*
 * TODO
 *  * main
 *  - [ ] error checking to see if read op actually read all of the file
 *  - [x] prepend the path to the file content
 *  - [ ] remove the process number
 */

static void *procsys_init(struct fuse_conn_info *conn,
                      struct fuse_config *cfg) {

    (void) conn;
    sleep(2); // to allow for server to start on run config
    cfg->use_ino = 1;

    /* Pick up changes from lower filesystem right away. This is
       also necessary for better hardlink support. When the kernel
       calls the unlink() handler, it does not know the inode of
       the to-be-removed entry and can therefore not invalidate
       the cache of the associated inode - resulting in an
       incorrect st_nlink value being reported for any remaining
       hardlinks to this inode. */
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;

    // init client
    server_sock = init_client(&server_addr);
    int pid;
    pipe(pipecomms);
    if((pid = fork()) < 0) {
        // error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // child
        close(pipecomms[0]); // reader process only needs to write to parent
        read_loop(server_sock);
    }
    close(pipecomms[1]); // parent only needs to listen to the reader process

    return NULL;
}

static int procsys_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {

    (void) fi;
    int res;
    const char *fpath = final_path(path);

    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;

    /*
     * proc files are 0 unless opened - can file contents change between now and a read call?
     * problem is that to cat a file we need the size > 0 here, but it takes a long time to ls -al
     * as it needs to get all filesizes.. Can we fill the stbuf elsewhere?
     */
    if((stbuf->st_mode & S_IFMT) == S_IFREG)
        stbuf->st_size = procsize(fpath);
//        stbuf->st_size = 0;

    return 0;
}

static int procsys_access(const char *path, int mask) {

    int res;

    res = access(final_path(path), mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int procsys_readlink(const char *path, char *buf, size_t size) {

    int res;

    res = readlink(final_path(path), buf, size - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}

static int procsys_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {

    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;
    (void) flags;

    dp = opendir(final_path(path));
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int procsys_open(const char *path, struct fuse_file_info *fi) {
    /*
     * TODO
     *  [?] Making this work with the new files -- check if procsys_read() is called during this op,
     *      if yes then ignore to-do
     *      - [ ] Must request the file from the server and return the pointer to this
     *          - [x] Make a method out of the process in read() to update the file to congregated one
     *          - [ ] Try using procsys_read() before opening
     *          - [ ] Make a new flag where the file content isn't sent with the flag&path
     */

    int res;
    const char *fpth = final_path(path);
    res = openat(AT_FDCWD, fpth, O_RDONLY);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int procsys_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    /*
     * TODO
     *  [ ] remove pid number from the path when 1< connected
     *  [ ] good memory management (calloc and realloc)
     */

    int fd;
    int res;
    char buffer[MAX] = {0};
    const char *fp = final_path(path);

    if(fi == NULL)
        fd = openat(AT_FDCWD, fp, O_RDONLY);
    else {
        fd = fi->fh;
    }
    if (fd == -1)
        return -errno;

    res = pread(fd, buffer, size, offset);
    if (res == -1)
        res = -errno;

    if(fi == NULL)
        close(fd);

    fetch_from_server(buffer, fp, buf, NME_MSG_CLI, server_sock, pipecomms[0]);
    return res;
}

static int procsys_statfs(const char *path, struct statvfs *stbuf) {

    int res;

    res = statvfs(final_path(path), stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int procsys_release(const char *path, struct fuse_file_info *fi) {

    (void) path;
    close(fi->fh);
    return 0;
}

static off_t procsys_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {

    int fd;
    off_t res;

    if (fi == NULL)
        fd = open(final_path(path), O_RDONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = lseek(fd, off, whence);
    if (res == -1)
        res = -errno;

    if (fi == NULL)
        close(fd);
    return res;
}

static const struct fuse_operations procsys_oper = {
        .init       = procsys_init,
        .getattr	= procsys_getattr,
        .access		= procsys_access,
        .readlink	= procsys_readlink,
        .readdir	= procsys_readdir,
        .open		= procsys_open,
        .read		= procsys_read,
        .statfs		= procsys_statfs,
        .release	= procsys_release,
        .lseek		= procsys_lseek,
};

int main(int argc, char *argv[]) {

    umask(0);
    return fuse_main(argc, argv, &procsys_oper, NULL);
}