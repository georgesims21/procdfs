#ifndef PROCSYS_SERVER_NEW_H
#define PROCSYS_SERVER_NEW_H

#include <glob.h>
#include <netinet/in.h>

#include "ds_new.h"

#define FREQ 1
#define FCNT 2
#define HEADER 166 // 32 IP + 16 port + 32 + 16 + 64 /*long long*/ + MAXPATH + 1 /*'\0'*/ + 7 /*'-'s*/;

int init_server(Address *address, int queue_length, int port_number, const char *interface);
/*
 * init server: - by main thread
 *  @param  queue length - given as argv to filesystem
 *          Address *server_addr
 *  @ret    error codes - print errors to log file too
 */

struct connect_to_file_IPs_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address host_addr;
    int arrlen;
    const char *filename;
};
void *connect_to_file_IPs(void *arg);
/*
 * connect to IPs: - by main thread 0
 *  @param  void* - pass connIPs_args struct
 *  @ret    NULL - we aren't joining main to this
 * TODO
 *      * more secure string checking on the file (remove whitespace etc)
 */

struct accept_connection_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address host_addr;
    int arrlen;
    const char *filename;
};
void *accept_connection(void *arg);
/*
 * accept_connection - by server thread n
 * @param   void* - pass accept_connection_args struct
 * @ret     NULL
 */

struct server_loop_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address host_addr;
    int arrlen;
    const char *filename;
};
void *server_loop(void *arg);
/*
 * server loop: - by server thread
 * @param   void* - pass server_loop_args struct
 * @ret     NULL - not joining main, it's a inf loop
 *
 * create fdset
 * for(;;):
 *      add sockets from connected clients[] - can extract the int sock
 *      listen to fdset
 *      noise on master:
 *      accept_connection()
 *      noise on fdset:
 *          client disconnect:
 *              disconnect_client()
 */
struct new_connection_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address host_addr;
    int arrlen;
    const char *filename;
};
char *create_message(Address host_addr, Request *req, int headerlen, int flag);
Inprog *inprog_create(const char *path);
void malloc_error(void);
void realloc_error(void);
void calloc_error(void);
int procsizefd(int fd);
void final_path(const char *path, char *buf);
Inprog *file_request(const char *path);
char *filebuf(const char *path);

#endif //PROCSYS_SERVER_NEW_H
