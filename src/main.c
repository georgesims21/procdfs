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
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
#include "reader.h"
#include "defs.h"

#include "ds_new.h"
#include "server_new.h"

int nrmachines;
long long a_counter;
pthread_mutex_t a_counter_lock = PTHREAD_MUTEX_INITIALIZER;
Address host_addr;
Address *connected_clients;
pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;;
Inprog_tracker_node *inprog_tracker_head;
pthread_mutex_t inprog_tracker_lock = PTHREAD_MUTEX_INITIALIZER;;

char *pnd = "/proc/net/dev";

static void *procsys_init(struct fuse_conn_info *conn,
                      struct fuse_config *cfg) {

    (void) conn;
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

    return NULL;
}

static int procsys_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {

    stbuf->st_gid = getgid();
    stbuf->st_uid = getuid();
    if(strcmp(path, "/") == 0) {
        // root
        stbuf->st_mode = S_IFDIR | 0444; // gives all read access no more
        stbuf->st_nlink = 2; // (2+n dirs) account for . and .. https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to/101536#101536
    } else if(strcmp(path, "/net/dev") == 0){
        // files
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1; // only located here, nowhere else yet
        stbuf->st_atim.tv_sec = time(NULL);
        // need to fetch the size of file
        Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
        memset(inprog, 0, sizeof(Inprog));
        inprog->complete = false;
        pthread_mutex_lock(&a_counter_lock);
        a_counter++;
        inprog->atomic_counter = a_counter;
        for(int i = 0; i < nrmachines; i++) {
            // create request
            Request *req = (Request *)malloc(sizeof(Request));
            memset(req, 0, sizeof(Request));
            // fill in request struct with info
            req->sender = connected_clients[i];
            req->atomic_counter = a_counter;
            pthread_mutex_unlock(&a_counter_lock);
            snprintf(req->path, MAXPATH, pnd);
            // Add to inprog
            pthread_mutex_lock(&inprog_tracker_lock);
            req_tracker_ll_add(&inprog->req_ll_head, req);
            inprog->messages_sent++;
            req_tracker_ll_print(&inprog->req_ll_head);
            pthread_mutex_unlock(&inprog_tracker_lock);
        }
        pthread_mutex_lock(&inprog_tracker_lock);
        inprog_ll_addnode(&inprog_tracker_head, inprog);
        pthread_mutex_unlock(&inprog_tracker_lock);
//        stbuf->st_size
    }

//    (void) fi;
//    int res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//
//    res = lstat(fpath, stbuf);
//    if (res == -1)
//        return -errno;
//
//    /*
//     * proc files are 0 unless opened - can file contents change between now and a read call?
//     * problem is that to cat a file we need the size > 0 here, but it takes a long time to ls -al
//     * as it needs to get all filesizes.. Can we fill the stbuf elsewhere?
//     */
//    if((stbuf->st_mode & S_IFMT) == S_IFREG)
//        stbuf->st_size = procsize(fpath);

    return 0;
}

static int procsys_access(const char *path, int mask) {

//    int res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//
//    res = access(fpath, mask);
//    if (res == -1)
//        return -errno;

    return 0;
}

static int procsys_readlink(const char *path, char *buf, size_t size) {

//    int res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//
//    res = readlink(fpath, buf, size - 1);
//    if (res == -1)
//        return -errno;
//
//    buf[res] = '\0';
    return 0;
}

static int procsys_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    (void)fi;

    filler(buf, ".", NULL, 0, 0); // Current Directory
    filler(buf, "..", NULL, 0, 0); // Parent Directory

    if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
    {
        filler(buf, "net/dev", NULL, 0, 0);
    }
//    DIR *dp;
//    struct dirent *de;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//
//    (void) offset;
//    (void) fi;
//    (void) flags;
//
//    dp = opendir(fpath);
//    if (dp == NULL)
//        return -errno;
//
//    while ((de = readdir(dp)) != NULL) {
//        struct stat st;
//        memset(&st, 0, sizeof(st));
//        st.st_ino = de->d_ino;
//        st.st_mode = de->d_type << 12;
//        if (filler(buf, de->d_name, &st, 0, 0))
//            break;
//    }
//
//    closedir(dp);
    return 0;
}

static int procsys_open(const char *path, struct fuse_file_info *fi) {

//    int res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//    res = openat(AT_FDCWD, fpath, O_RDONLY);
//    if (res == -1)
//        return -errno;
//
//    fi->fh = res;
    return 0;
}

static int procsys_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    /*
     * TODO
     *  [ ] remove pid number from the path when 1< connected
     *  [ ] good memory management (calloc and realloc)
     */

//    int fd;
//    int res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//    remove_pid(fpath);
////    const char *fp = fpath;
//
//    if(fi == NULL)
//        fd = openat(AT_FDCWD, fpath, O_RDONLY);
//    else {
//        fd = fi->fh;
//    }
//    if (fd == -1)
//        return -errno;
//
//    int filesize = procsizefd(fd);
////    char *buffer = malloc((filesize * sizeof(char)) + 1);
//    char buffer[MAX] = {0};
//    res = pread(fd, buffer, size, offset);
//    buffer[filesize + 1] = '\0';
//    if (res == -1)
//        res = -errno;
//
//    if(fi == NULL)
//        close(fd);
//
////    fetch_from_server(buffer, fpath, buf, NME_MSG_CLI, client_socket, pipecomms[0]);
////    free(buffer);
//
//    fetch_from_server(fpath, &buf, NME_MSG_CLI, client_socket, pipecomms[0]);
//    return res;
}

static int procsys_statfs(const char *path, struct statvfs *stbuf) {

//    int res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//
//    res = statvfs(fpath, stbuf);
//    if (res == -1)
//        return -errno;
//
    return 0;
}

static int procsys_release(const char *path, struct fuse_file_info *fi) {

//    (void) path;
//    close(fi->fh);
    return 0;
}

static off_t procsys_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {

//    int fd;
//    off_t res;
//    char fpath[MAX_PATH] = {0};
//    final_path(path, fpath);
//
//    if (fi == NULL)
//        fd = open(fpath, O_RDONLY);
//    else
//        fd = fi->fh;
//
//    if (fd == -1)
//        return -errno;
//
//    res = lseek(fd, off, whence);
//    if (res == -1)
//        res = -errno;
//
//    if (fi == NULL)
//        close(fd);
//    return res;
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
    if(argc < 6) {
        printf("Not enough arguments given, at least 5 expected: "
               "mountpoint total-machines port-number interface-name ipfile\n");
        exit(EXIT_FAILURE);
    }
    printf("argc: %d\n", argc);
    argc--;
    const char *fn = argv[argc--];
    const char *infc = argv[argc--];
    long pnr = strtol(argv[argc--], NULL, 10);
    long nrm = strtol(argv[argc], NULL, 10);
    // Check if returned error from strtol OR if the longs are too large to convert
    if (errno != 0 || ((nrm > INT_MAX) || (pnr > INT_MAX ))) {
        printf("%s argument too large!\n", (nrm > INT_MAX) ? "first" : "second");
        exit(EXIT_FAILURE);
    }
    int err;
    nrmachines = (int)nrm - 1; // to account for this machine (not adding to connected clients)
    int portnr = (int)pnr;

    umask(0);
    printf("Connecting to other machines..\n");
    memset(&host_addr, 0, sizeof(host_addr));
    init_server(&host_addr, nrmachines, portnr, infc);

    // init Address arrays and their corresponding mutex locks
    connected_clients = (Address *)malloc(sizeof(Address) * nrmachines);
    memset(connected_clients, 0, sizeof(Address) * nrmachines);

    // accept all incoming connections until have sock_in for all machines in list
    struct accept_connection_args aca = {connected_clients, &connected_clients_lock,
                                         host_addr, nrmachines, fn};
    pthread_t aca_thread;
    pthread_create(&aca_thread, NULL, accept_connection, &aca);
    // connect to IPs in ipfile
    struct connect_to_file_IPs_args ctipa = {connected_clients, &connected_clients_lock,
                                             host_addr, nrmachines, fn};
    pthread_t ctipa_thread;
    pthread_create(&ctipa_thread, NULL, connect_to_file_IPs, &ctipa);
    // force main to wait until connected to all machines
    pthread_join(ctipa_thread, NULL);
    pthread_join(aca_thread, NULL);

    printf("You are now connected to machines: \n");
    for(int j = 0; j < nrmachines; j++) {
        printf("%s\t@\t%d\n",
               inet_ntoa(connected_clients[j].addr.sin_addr),
               htons(connected_clients[j].addr.sin_port));
    }
    printf("on this address: \n%s\t@\t%d\n",
           inet_ntoa(host_addr.addr.sin_addr),
           htons(host_addr.addr.sin_port));

    // start server loop to listen for connections
    struct server_loop_args sla = {connected_clients, &connected_clients_lock, host_addr,
                                   inprog_tracker_head, &inprog_tracker_lock,
                                    nrmachines, fn};
    pthread_t sla_thread;
    pthread_create(&sla_thread, NULL, server_loop, &sla);

    return fuse_main(argc, argv, &procsys_ops, NULL);
}