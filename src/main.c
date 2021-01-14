/*
 * Main skeleton taken from the libfuse example: passthrough.c
 */
#define FUSE_USE_VERSION 35
#define _GNU_SOURCE

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#ifdef __FreeBSD__
#include <sys/socket.h>
#include <sys/un.h>
#endif
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "writer.h"
#include "ds_new.h"
#include "server_new.h"
#include "pathnames.h"

int nrmachines;
long long a_counter;
Address host_addr;
Address *connected_clients;
Inprog_tracker_node *inprog_tracker_head;
pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inprog_tracker_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t a_counter_lock = PTHREAD_MUTEX_INITIALIZER;

static int procsys_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {

    (void)fi;
    char pathbuf[MAXPATH] = {0};
    stbuf->st_gid = getgid();
    stbuf->st_uid = getuid();
    stbuf->st_atim.tv_sec = time(NULL);
    stbuf->st_mtim.tv_sec = time(NULL);
    stbuf->st_ctim.tv_sec = time(NULL);
    if(strcmp(path, "/") == 0) {
        // root
        stbuf->st_mode = S_IFDIR | 0444; // gives all read access no more
        stbuf->st_nlink = 3; // (2+n dirs) account for . and .. https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to/101536#101536
    }
    else if(strcmp(path, "/net") == 0) {
        stbuf->st_mode = S_IFDIR | 0444;
        stbuf->st_nlink = 2;
    } else {
        final_path(path, pathbuf);
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1; // only located here, nowhere else yet
        for(int i = 0; i < PATHARRLEN; i++) {
            if(strcmp(path, paths[i]) == 0) {
                if(nrmachines == 0) {
                    int fd = -1, res = 0, offset = 0, size = 0, err = 0;
                    fd = openat(-100, pathbuf, O_RDONLY);
                    if (fd == -1) {
                        perror("openat");
                        printf("%s\n", pathbuf);
                        exit (EXIT_FAILURE);
                    }
                    size = procsizefd(fd); // individually count chars in proc file - bottleneck for large fs
                    char *procbuf = malloc(sizeof(char) * size);
                    if(!procbuf){malloc_error();};
                    res = pread(fd, procbuf, size, offset);
                    if (res == -1) {
                        perror("pread");
                        exit(EXIT_FAILURE);
                    }
                    close(fd);
                    stbuf->st_size = size;
                    break;
                }
                Inprog *inprog = file_request(pathbuf);
                size_t buflen = request_ll_countbuflen(&inprog->req_ll_head);
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
                           off_t offset, struct fuse_file_info *fi) {

    (void)offset;
    (void)fi;

    filler( buf, ".", NULL, 0); // Current Directory
    filler( buf, "..", NULL, 0); // Parent Directory

    if (strcmp(path, "/") == 0) {
        filler(buf, "net", NULL, 0);
    }
    if (strcmp(path, "/net") == 0) {
        for(int i = 0; i < PATHARRLEN; i++) {
            filler(buf, filenames[i], NULL, 0);
        }

    }
    return 0;
}

static int procsys_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {

    (void)fi;
    unsigned int buflen;
    char *filebuf;
    char pathbuf[MAXPATH] = {0};
    final_path(path, pathbuf);
    for(int i = 0; i < PATHARRLEN; i++) {
        if(strcmp(path, paths[i]) == 0) {
            if(nrmachines == 0) {
                int fd = -1, res = 0, offset = 0, size = 0, err = 0;
                fd = openat(-100, pathbuf, O_RDONLY);
                if (fd == -1) {
                    perror("openat");
                    printf("%s\n", pathbuf);
                    exit (EXIT_FAILURE);
                }
                size = procsizefd(fd); // individually count chars in proc file - bottleneck for large fs
                filebuf = malloc(sizeof(char) * size);
                if(!filebuf){malloc_error();};
                res = pread(fd, filebuf, size, offset);
                if (res == -1) {
                    perror("pread");
                    exit(EXIT_FAILURE);
                }
                close(fd);
                buflen = strlen(filebuf);
                goto single_machine_tmp;
            }
            char pathbuftmp[MAXPATH] = {0};
            final_path(path, pathbuftmp);
            Inprog *inprog = file_request(pathbuf);
            filebuf = request_ll_catbuf(&inprog->req_ll_head);
            buflen = strlen(filebuf);
            pthread_mutex_lock(&inprog_tracker_lock);
            // delete inprog from list
            inprog_tracker_ll_remove(&inprog_tracker_head, *inprog);
            pthread_mutex_unlock(&inprog_tracker_lock);
            single_machine_tmp:
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
        .getattr	= procsys_getattr,
        .access		= procsys_access,
        .readlink	= procsys_readlink,
        .readdir	= procsys_readdir,
        .open		= procsys_open,
        .read		= procsys_read,
        .statfs		= procsys_statfs,
        .release	= procsys_release,
};

int main(int argc, char *argv[]) {

    if(argc < 6) {
        printf("Not enough arguments given, at least 5 expected: "
               "[fuse flags] mountpoint total-machines port-number interface-name ipfile\n");
        exit(EXIT_FAILURE);
    }
    argc--;
    const char *ip = argv[argc--];
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
    memset(&host_addr, 0, sizeof(host_addr));
    host_addr.addr.sin_addr.s_addr = inet_addr(ip);
    init_server(&host_addr, nrmachines, portnr, infc, ip);

    if(nrm == 0)
        goto single_machine;

    printf("Connecting to other machines..\n");
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
        printf("%s\t@\t%d\nsock_in: %d\tsock_out: %d\n",
               inet_ntoa(connected_clients[j].addr.sin_addr),
               htons(connected_clients[j].addr.sin_port),
               connected_clients[j].sock_in,
               connected_clients[j].sock_out);
    }
    printf("on this address: \n%s\t@\t%d\nsock_in: %d\n",
           inet_ntoa(host_addr.addr.sin_addr),
           htons(host_addr.addr.sin_port),
           host_addr.sock_in);

    pthread_mutex_init(&connected_clients_lock, NULL);
    pthread_mutex_init(&inprog_tracker_lock, NULL);
    pthread_mutex_init(&a_counter_lock, NULL);

    // start server loop to listen for connections
    struct server_loop_args sla = {connected_clients, &connected_clients_lock,
                                   host_addr, nrmachines, fn};
    pthread_t sla_thread;
    pthread_create(&sla_thread, NULL, server_loop, &sla);

    single_machine:
    return fuse_main(argc, argv, &procsys_ops, NULL);
}
