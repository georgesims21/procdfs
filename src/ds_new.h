#ifndef PROCSYS_DS_NEW_H
#define PROCSYS_DS_NEW_H

#include <glob.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>

#define MAXPATH 64

typedef struct address {
    int sock_in;  // recv
    int sock_out; // send
    size_t addr_len;
    struct sockaddr_in addr;
}Address;

typedef struct request {
    Address sender;
    long long atomic_counter;
    bool complete;
    char path[MAXPATH];
    unsigned long buflen;
    char buf[]; // to allow for flexible array member
}Request;

typedef struct request_tracker_node {
    Request *req; // may need to be pointer
    struct request_tracker_node *next;
}Request_tracker_node;

typedef struct inprog {
    long long atomic_counter;
    int messages_sent;
    bool complete;
    Request_tracker_node *req_ll_head; // head of linked list of the machines who got message
}Inprog;

typedef struct inprog_tracker_node {
    Inprog *inprog;
    pthread_mutex_t *inprog_lock;
    struct inprog_tracker_node *next;
}Inprog_tracker_node;

extern int nrmachines;
extern long long a_counter;
extern Address host_addr;
extern Address *connected_clients;
extern Inprog_tracker_node *inprog_tracker_head;
extern pthread_mutex_t connected_clients_lock;
extern pthread_mutex_t inprog_tracker_lock;
extern pthread_mutex_t a_counter_lock;

int req_tracker_ll_add(Request_tracker_node **head, Request *req);
void req_tracker_ll_print(Request_tracker_node **head);
int req_add_content(Request_tracker_node **head, Request req, char *cnt, int cntlen);
Request_tracker_node *req_tracker_ll_fetch(Request_tracker_node **head, Request req);
void inprog_free(Inprog *inp);
int request_ll_free(Request_tracker_node **head);
/* To allow reuse of one request structure, before needing to implement
 * the inprog tracker linked list at a later time
 */
void inprog_reset(Inprog *inp);
int request_ll_complete(Request_tracker_node **head);
int inprog_add_buf(Request *req, Inprog *inprog, pthread_mutex_t *inprog_lock);
int inprog_tracker_ll_add(Inprog_tracker_node **head, Inprog *inprog, pthread_mutex_t *inprog_lock);
void inprog_tracker_ll_print(Inprog_tracker_node **head);
Inprog_tracker_node *inprog_tracker_ll_fetch_req(Inprog_tracker_node **head, Request req);
Inprog_tracker_node *inprog_tracker_ll_fetch_node(Inprog_tracker_node **head, Inprog inprog);
int inprog_tracker_ll_remove(Inprog_tracker_node **head, Inprog inprog);

#endif //PROCSYS_DS_NEW_H
