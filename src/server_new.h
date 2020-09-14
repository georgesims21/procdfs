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
    Inprog *inprog; pthread_mutex_t *inprog_lock;
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
 * TODO
 *          *need to think about send/recv not reading/sending all bytes in one call!:*
     *          Create the header ()
         *          need hostip and port
         *          receiverip and port
         *          atomic counter
         *          filepath
     *          Add flag ()
     *          Add payload () - if request was recieved
     *          Get message size ()
     *          put them together and send
 *          parse flag()
 *          file request:
 *              malloc Request fr;
 *              extract_file_request(fr, host_addr)
 *              fetch file contents:
 *                  remember to realloc fr with size of file and save into buf
 *              malloc buffer to send over socket with length of fr
 *              create_request(fr, frlen, buf, buflen)
 *              free(fr)
 *              send request to caller
 *              free(buf)
 *          file content:
 *              *message request struct already exists for this*
 *              malloc Request fc;
 *              extract_content_request(fc, host_addr)
 *              mark_as_received(fc, inpr_ll_ptr, inprog_tracker_ll_lock)
 *
 */

// statics
struct new_connection_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address host_addr;
    int arrlen;
    const char *filename;
};
static void *new_connection(void *arg);
static int fetch_IP(Address *addr, const char *interface);
static int next_space(Address *addr, int addrlen);
static int pconnect(Address *addrarr, int arrlen, Address *newaddr);
static int contained_within(Address *addr, Address lookup, int arrlen);
static void paccept(Address host_addr, Address *client_addr);
static int search_IPs(in_addr_t conn, const char *filename);

/*
 * struct create_request_args {
 *      struct Request *req;
 *      int req_len; // length of request struct
 *      char *content; // malloc'ed
 *      int con_len; // length of content buffer
 * };
 */

/*
 * STATIC create_request - by server thread n
 * @param   void* - pass create_request_args struct
 * @ret     NULL
 *
 * convert all ints to chars like I did with flags
 * convert given Request struct into a buffer
 * https://stackoverflow.com/questions/8000851/passing-a-struct-over-tcp-sock-stream-socket-in-c
 */


/*
 * struct mark_as_received_args {
 *      struct Request *req;
 *      int req_len; // length of request struct
 *      mutex_lock_t inprog_tracker_ll_lock;
 *      int *inpr_ll_ptr; // head of Inprog_tracker_ll
 * };
 */

/*
 * STATIC mark_as_received - by server thread n
 * @param   void* - pass mark_as_received_args struct
 * @ret     NULL
 *
 *      mutex_lock(inprog_tracker_ll_lock)
 *      create iterator pointers **inprgpt, **reqpt, **reqpt2;
 *      find correct request struct using counter (req->atomiccounter == *inprgpt->inpt->atomiccounter)
 *      point *reqpt to *inprgpt->inpt->*req_ll_ptr
 *      find correct sender IP (req->...->sin_addr == *reqpt->inpt->req->sender->addr->sin_addr->sin_addr)
 *      check correct sender port (req->...->sin_port == *reqpt->inpt->req->sender->addr->sin_port)
 *      add file content to buf (req->buf = *reqpt->inpt->req->buf)
 *      mark as received (*reqpt->inpt->req->received = 1)
 *      point *reqpt2 to *inprgpt->inpt->*req_ll_ptr
 *          while(*reqpt2->next =! NULL)
 *          if(*reqpt2->req->received == 1)
 *              count++
 *          if(count == *inprgpt->sent_msgs)
 *              *inprgpt->inpt->complete = 1
 *      mutex_unlock(inprog_tracker_ll_lock)
 */

#endif //PROCSYS_SERVER_NEW_H
