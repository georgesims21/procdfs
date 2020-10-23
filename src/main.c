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

#include "log.h"
#include "writer.h"
#include "ds_new.h"
#include "server_new.h"
#include "pathnames.h"
#include "fileops_new.h"

int nrmachines;
long long a_counter;
Address host_addr;
Address *connected_clients;
Inprog_tracker_node *inprog_tracker_head;
pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inprog_tracker_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t a_counter_lock = PTHREAD_MUTEX_INITIALIZER;

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
    /*
     * Files start with '/' in the path
     */
    (void)fi;

    stbuf->st_gid = getgid();
    stbuf->st_uid = getuid();
    stbuf->st_atim.tv_sec = time(NULL);
    if(strcmp(path, "/") == 0) {
        // root
        stbuf->st_mode = S_IFDIR | 0444; // gives all read access no more
        stbuf->st_nlink = 3; // (2+n dirs) account for . and .. https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to/101536#101536
    }
    else if(strcmp(path, "/net") == 0) {
        stbuf->st_mode = S_IFDIR | 0444;
        stbuf->st_nlink = 2;
    } else {
        char pathbuf[MAXPATH] = {0};
        final_path(path, pathbuf);
        printf("getattr called on (pathbuf): %s\n", pathbuf);
        // files
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1; // only located here, nowhere else yet
        /* Important we lock here, as the server thread will try access ll once
         * messages are received from sender machines, if this is slow could cause race conditions */
        for(int i = 0; i < PATHARRLEN; i++) {
            if(strcmp(path, paths[i]) == 0) {
                Inprog *inprog = file_request(pathbuf);
                if(strcmp(paths[i], _PATH_PROCNET_DEV) == 0) {
                    char newmatrix[32][32][128] = {0};
                    int row_count = 0;
                    /* need to run this in loop with all request reply buffers, merging with
                    the newmatrix each time. i.e merge (reqbuf[i], newmatrix, newmatrix). Also
                     need to get this machines file and turn into matrix, best to do last out of loop */
                    for(int j = 0; j < inprog->messages_sent; j++) {
                        char matrix[32][32][128] = {0};
                        // needs to iterate over all of the request bufs
                        procnet_dev_extract(inprog->req_ll_head->req->buf, matrix);
                        procnet_dev_merge(matrix, newmatrix, newmatrix, &row_count);
                    }
                    char matrix2[32][32][128] = {0};
                    char newmatrix2[32][32][128] = {0};
                    char *hostdev = filebuf(pathbuf);
                    procnet_dev_extract(hostdev, matrix2);
                    procnet_dev_merge(matrix2, newmatrix, newmatrix, &row_count);
                    for(int j = 0; j < 32; j++) {
                        for(int k = 0; k < 17; k++) {
                            printf("%s", newmatrix[j][k]);
                        }
                    }
                    printf("\n");
//                    printf("(newmatrix) %s transmitted %s = %s\n", newmatrix[2][0], newmatrix[1][9], newmatrix[2][9]);
                    exit(EXIT_SUCCESS);
                }

                size_t buflen = request_ll_countbuflen(&inprog->req_ll_head);
//            inprog_tracker_ll_print(&inprog_tracker_head);
                pthread_mutex_lock(&inprog_tracker_lock);
                // delete inprog from list
                inprog_tracker_ll_remove(&inprog_tracker_head, *inprog);
                pthread_mutex_unlock(&inprog_tracker_lock);
                stbuf->st_size = buflen;
            }
        }
    }
    return 0;
}

static int procsys_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi,
                           enum fuse_readdir_flags flags) {
    (void)offset;
    (void)fi;
    (void)flags;
    printf("readdir\n");

    filler( buf, ".", NULL, 0, 0); // Current Directory
    filler( buf, "..", NULL, 0, 0); // Parent Directory

    if (strcmp(path, "/") == 0) {
        filler(buf, "net", NULL, 0, 0);
    }
    if (strcmp(path, "/net") == 0) {
        for(int i = 0; i < PATHARRLEN; i++) {
            filler(buf, filenames[i], NULL, 0, 0);
        }

    }
    return 0;
}

static int procsys_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    (void)fi;
    printf("\n\nreading file: %s\noffset: %ld\nsize: %lu\n", path, offset, size);

    for(int i = 0; i < PATHARRLEN; i++) {
        if(strcmp(path, paths[i]) == 0) {
            char pathbuf[MAXPATH] = {0};
            final_path(path, pathbuf);
            printf("getattr called on : %s\n", pathbuf);
            Inprog *inprog = file_request(pathbuf);

            char *filebuf = request_ll_catbuf(&inprog->req_ll_head);
            unsigned int buflen = strlen(filebuf);
//        inprog_tracker_ll_print(&inprog_tracker_head);
            pthread_mutex_lock(&inprog_tracker_lock);
            // delete inprog from list
            inprog_tracker_ll_remove(&inprog_tracker_head, *inprog);
            pthread_mutex_unlock(&inprog_tracker_lock);
            if (offset >= buflen) {
                free(filebuf);
                return 0;
            }

            if (offset + size > buflen) {
                memcpy(buf, filebuf + offset, buflen - offset);
                free(filebuf);
                return buflen - offset;
            }

            memcpy(buf, filebuf + offset, size);
            free(filebuf);
            return size;
        }
    }
    return -ENOENT;
}

static int procsys_access(const char *path, int mask) {

    (void)path;
    (void)mask;
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

    (void)path;
    (void)buf;
    (void)size;
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


static int procsys_open(const char *path, struct fuse_file_info *fi) {

    (void)path;
    (void)fi;
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

static int procsys_statfs(const char *path, struct statvfs *stbuf) {

    (void)path;
    (void)stbuf;

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

    (void)path;
    (void)fi;
//    (void) path;
//    close(fi->fh);
    return 0;
}

static off_t procsys_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {

    (void)path;
    (void)off;
    (void)whence;
    (void)fi;
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
    return 0;
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
               "[fuse flags] mountpoint total-machines port-number interface-name ipfile\n");
        exit(EXIT_FAILURE);
    }
    argc--;
    const char *fn = argv[argc--];
    const char *infc = argv[argc--];
    long pnr = strtol(argv[argc--], NULL, 10);
    long nrm = strtol(argv[argc], NULL, 10);
    // Check if returned error from strtol OR if the longs are too large to convert
    if (errno != 0 || ((nrm > INT_MAX) || (pnr > INT_MAX ))) {
        perror("error:");
        printf("%s argument too large!\n", (nrm > INT_MAX) ? "first" : "second");
        exit(EXIT_FAILURE);
    }
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

    pthread_mutex_init(&connected_clients_lock, NULL);
    pthread_mutex_init(&inprog_tracker_lock, NULL);
    pthread_mutex_init(&a_counter_lock, NULL);

    // start server loop to listen for connections
    struct server_loop_args sla = {connected_clients, &connected_clients_lock,
                                   host_addr, nrmachines, fn};
    pthread_t sla_thread;
    pthread_create(&sla_thread, NULL, server_loop, &sla);

    printf("\n----- starting fuse -----\n");

    return fuse_main(argc, argv, &procsys_ops, NULL);
}