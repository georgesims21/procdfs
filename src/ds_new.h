#ifndef PROCSYS_DS_NEW_H
#define PROCSYS_DS_NEW_H

#include <glob.h>
#include <netinet/in.h>

#define MAXPATH 64

typedef struct address {
    int sock;
    size_t addr_len;
    struct sockaddr_in addr;
}Address;

typedef struct request {
    Address sender;
    char received;
    int atomic_counter;
    char path[MAXPATH];
    char buf[]; // to allow for flexible array member
}Request;

typedef struct request_tracker_node {
    Request *req; // may need to be pointer
    struct request_tracker_node *next;
}Request_tracker_node;

typedef struct inprog {
    int atomic_counter;
    int messages_sent;
    char complete;
    Request_tracker_node *req_ll_head; // head of linked list of the machines who got message
}Inprog;

typedef struct inprog_tracker_node {
    Inprog *inprg;
    struct inprog_tracker_node *next;
}Inprog_tracker_node;


int req_tracker_ll_add(Request_tracker_node **rtn, Request *req);

#endif //PROCSYS_DS_NEW_H
