#include "ds_new.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

int req_tracker_ll_add(Request_tracker_node **head, Request *req) {

    if(req == NULL)
        return -1;
    Request_tracker_node *listptr = *head;
    Request_tracker_node *newnode = (Request_tracker_node *)malloc(sizeof(Request_tracker_node));
    if(newnode == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(newnode, 0, sizeof(Request_tracker_node));
    newnode->req = req;
    newnode->next = NULL;
    if(listptr == NULL) {
        // create new head of list
        *head = newnode;
    } else {
        // append to end of list
        while(listptr->next != NULL)
            // should we check if exists here too?
            listptr = listptr->next;
        listptr->next = newnode;
    }
    return 0;
}

int request_cmp(Request req1, Request req2) {

    if (req1.atomic_counter == req2.atomic_counter &&
            strncmp(req1.path, req2.path, strlen(req1.path)) == 0 &&
            req1.sender.addr.sin_addr.s_addr == req2.sender.addr.sin_addr.s_addr) {
        return 0;
    }
    return 1;
}

int req_add_content(Request_tracker_node **head, Request req, char *cnt, int cntlen) {
    // assumes user gave size INCLUDING '\0'

    Request_tracker_node *node = req_tracker_ll_fetch(head, req);
    if(node == NULL)
        return -1;
    node->req = realloc(node->req, sizeof(Request) + (cntlen * sizeof(char)));
    if(node->req == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    snprintf(node->req->buf, cntlen, "%s", cnt);
    node->req->buflen = cntlen;
    return 0;
}

Request_tracker_node *req_tracker_ll_fetch(Request_tracker_node **head, Request req) {

    if(head == NULL)
        return NULL;
    Request_tracker_node *listptr = *head;
    while(listptr != NULL) {
        if(request_cmp(*listptr->req, req) == 0) {
            return listptr;
        }
        listptr = listptr->next;
    }
    return NULL;
}

void req_tracker_ll_print(Request_tracker_node **head) {

    int count = 0;
    Request_tracker_node *listptr = *head;

    if(listptr != NULL) {
        while (listptr != NULL) {
            printf("req_tracker_ll[%d]:\nAtomic counter: %llu\n"
                   "File path: %s\n"
                   "Sock_in: %d\n"
                   "Sock_out: %d\n"
                   "IP: %s\n"
                   "Port: %u\n"
                   "Buflen: %lu\n"
                   "Sender buf: %s\n"
                   "\n",
                   count,
                   listptr->req->atomic_counter,
                   listptr->req->path,
                   listptr->req->sender.sock_in,
                   listptr->req->sender.sock_out,
                   inet_ntoa(listptr->req->sender.addr.sin_addr),
                   htons(listptr->req->sender.addr.sin_port),
                   listptr->req->buflen,
                   (listptr->req->buflen > 0) ? listptr->req->buf : ""
            );
            listptr = listptr->next;
            count++;
        }
        return;
    }
    printf("List empty!\n");
}

int request_ll_free(Request_tracker_node **head) {

    Request_tracker_node *reqptr = *head;
    Request_tracker_node *next;
    if(reqptr == NULL) {
        printf("The list is empty! Not freeing anything\n");
        return -1;
    }
    while(reqptr != NULL) {
        if(reqptr->req != NULL) {
            free(reqptr->req);
        }
        next = reqptr->next;
        free(reqptr);
        reqptr = next;
    }
    return 0;
}

int request_ll_complete(Request_tracker_node **head) {

    Request_tracker_node *reqptr = *head;
    Request_tracker_node *next;
    int count = 0;
    if(reqptr == NULL) {
        printf("The list is empty! Not freeing anything\n");
        return -1;
    }
    while(reqptr != NULL) {
        if(reqptr->req != NULL) {
            if(reqptr->req->complete)
                count++;
        }
        next = reqptr->next;
        reqptr = next;
    }
    return count;
}

int inprog_add_buf(Request *req, Inprog *inprog, pthread_mutex_t *inprog_lock) {

    Request_tracker_node *rtn;
    pthread_mutex_lock(inprog_lock);
    if((rtn = req_tracker_ll_fetch(&inprog->req_ll_head, *req)) != NULL) {
        // Realloc this rtn and copy buflen and buf into it, now it is complete
        printf("Request found!\n");
        rtn->req->buflen = req->buflen;
        rtn->req = realloc(rtn->req, sizeof(Request) + req->buflen);
        memset(rtn->req->buf, 0, req->buflen);
        strcpy(rtn->req->buf, req->buf);
        rtn->req->complete = true;
        printf("After adding buffer:\n");
        req_tracker_ll_print(&inprog->req_ll_head);
        // check if all requests (inc. this one) have been received
        if(request_ll_complete(&inprog->req_ll_head) == inprog->messages_sent) {
            inprog->complete = true;
            printf("All messages received! Resetting inprog\n");
            inprog_reset(inprog);
        }
    } else {
        printf("Request not contained within the linked list, exiting...\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(inprog_lock);
    return 0;
}

void inprog_reset(Inprog *inp) {

    request_ll_free(&inp->req_ll_head);
    memset(inp, 0, sizeof(Inprog));
}
