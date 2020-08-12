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
#include "server.h"
#include "writer.h"
#include "reader.h"
#include "defs.h"

int client_socket;
int pipecomms[2];

/*
 * TODO
 *  * main
 *  - [ ] error checking to see if read op actually read all of the file
 *  - [x] prepend the path to the file content
 *  - [x] remove the process number
 *  * bugs
 *  - [x] if typed 'cat <filename>' and then delete a char, main segfaults (seems to be only net/dev)
 *      * was the malloc in read()
 *      * Can we somehow skip the reading of the file and get res somewhere else?
 */

static void *procsys_init(struct fuse_conn_info *conn,
                      struct fuse_config *cfg) {

    (void) conn;
    struct sockaddr_in server_addr;
    int server_socket;
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

    // if address is already in use then don't run a server
    if((server_socket = init_server(10, &server_addr)) > -1) {
        int pid;
        if((pid = fork()) < 0) {
            // error
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // child
            server_loop(server_socket, sizeof(server_addr), &server_addr);
        }
    }

    // init client
    client_socket = init_client(&server_addr);
    int pid;
    pipe(pipecomms);
    if((pid = fork()) < 0) {
        // error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // child
        close(pipecomms[0]); // reader process only needs to write to parent
        read_loop(client_socket, pipecomms[1]);
    }
    close(pipecomms[1]); // parent only needs to listen to the reader process

    return NULL;
}

static int procsys_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {

    (void) fi;
    int res;
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);

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

    return 0;
}

static int procsys_access(const char *path, int mask) {

    int res;
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);

    res = access(fpath, mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int procsys_readlink(const char *path, char *buf, size_t size) {

    int res;
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);

    res = readlink(fpath, buf, size - 1);
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
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);

    (void) offset;
    (void) fi;
    (void) flags;

    dp = opendir(fpath);
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

    int res;
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);
    res = openat(AT_FDCWD, fpath, O_RDONLY);
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
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);
    remove_pid(fpath);
//    const char *fp = fpath;

    if(fi == NULL)
        fd = openat(AT_FDCWD, fpath, O_RDONLY);
    else {
        fd = fi->fh;
    }
    if (fd == -1)
        return -errno;

    int filesize = procsizefd(fd);
//    char *buffer = malloc((filesize * sizeof(char)) + 1);
    char buffer[MAX] = {0};
    res = pread(fd, buffer, size, offset);
    buffer[filesize + 1] = '\0';
    if (res == -1)
        res = -errno;

    if(fi == NULL)
        close(fd);

//    fetch_from_server(buffer, fpath, buf, NME_MSG_CLI, client_socket, pipecomms[0]);
//    free(buffer);

    fetch_from_server(fpath, &buf, NME_MSG_CLI, client_socket, pipecomms[0]);
    return res;
}

static int procsys_statfs(const char *path, struct statvfs *stbuf) {

    int res;
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);

    res = statvfs(fpath, stbuf);
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
    char fpath[MAX_PATH] = {0};
    final_path(path, fpath);

    if (fi == NULL)
        fd = open(fpath, O_RDONLY);
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

static const struct fuse_operations procsys_ops = {
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
    return fuse_main(argc, argv, &procsys_ops, NULL);
}