#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "inprog.h"
#include "request.h"

int inprog_add_buf(Request *req, Inprog *inprog, pthread_mutex_t *inprog_lock) {

    Request_tracker_node *rtn;
    pthread_mutex_lock(inprog_lock);
    if((rtn = req_tracker_ll_fetch(&inprog->req_ll_head, *req)) != NULL) {
        // Realloc this rtn and copy buflen and buf into it, now it is complete
        rtn->req->buflen = req->buflen;
        rtn->req = realloc(rtn->req, sizeof(Request) + req->buflen);
        memset(rtn->req->buf, 0, req->buflen);
        strcpy(rtn->req->buf, req->buf);
        rtn->req->complete = true;
        // check if all requests (inc. this one) have been received
        pthread_mutex_lock(inprog->complete_lock);
        if(request_ll_complete(&inprog->req_ll_head) == inprog->messages_sent) {
            inprog->complete = true;
            pthread_cond_broadcast(inprog->complete_cond);
            printf("All messages received!\n");
        }
        pthread_mutex_unlock(inprog->complete_lock);
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

int inprog_tracker_ll_add(Inprog_tracker_node **head, Inprog *inprog) {

    if(inprog == NULL)
        return -1;
    Inprog_tracker_node *listptr = *head;
    Inprog_tracker_node *newnode = (Inprog_tracker_node *)malloc(sizeof(Inprog_tracker_node));
    if(newnode == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(newnode, 0, sizeof(Request_tracker_node));
    newnode->inprog = inprog;
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

void inprog_tracker_ll_print(Inprog_tracker_node **head) {

    if(head == NULL) {
        printf("List empty!\n");
        return;
    }
    int count = 0;
    Inprog_tracker_node *listptr = *head;

    if(listptr != NULL) {
        while (listptr != NULL) {
            printf("inprog_tracker_ll[%d]:\nAtomic counter: %llu\n"
                   "Complete: %d\n"
                   "Messages sent: %d\n"
                   "\n",
                   count,
                   listptr->inprog->atomic_counter,
                   listptr->inprog->complete,
                   listptr->inprog->messages_sent
            );
            listptr = listptr->next;
            count++;
        }
        return;
    }
}

Inprog_tracker_node *inprog_tracker_ll_fetch_req(Inprog_tracker_node **head, Request req) {

    Inprog_tracker_node *listptr = *head;
    if(head == NULL || listptr == NULL || strcmp(req.path, "") == 0) {
        printf("List is empty/request is empty, exiting..\n");
        exit(EXIT_FAILURE);
    }
    while(listptr != NULL) {
        if(listptr->inprog->atomic_counter == req.atomic_counter) {
            return listptr;
        }
        if(listptr->next != NULL) {
            listptr = listptr->next;
        } else {
            break;
        }
    }
    printf("Cannot find request, exiting..\n");
    exit(EXIT_FAILURE);
}

Inprog_tracker_node *inprog_tracker_ll_fetch_node(Inprog_tracker_node **head, Inprog inprog) {

    if(head == NULL || inprog.req_ll_head == NULL) {
        printf("List is empty/inprog list is empty, exiting..\n");
        exit(EXIT_FAILURE);
    }
    Inprog_tracker_node *listptr = *head;
    while(listptr != NULL) {
        if(listptr->inprog->atomic_counter == inprog.atomic_counter) {
            return listptr;
        }
        if(listptr->next != NULL) {
            listptr = listptr->next;
        } else {
            break;
        }
    }
    printf("Cannot find inprog, exiting..\n");
    exit(EXIT_FAILURE);
}

int inprog_tracker_ll_remove(Inprog_tracker_node **head, Inprog inprog) {

    Inprog_tracker_node *reqptr = *head;
    Inprog_tracker_node *tmp;
    if(reqptr == NULL) {
        printf("The list is empty! Not freeing anything\n");
        return -1;
    } else if (inprog.req_ll_head == NULL) {
        printf("Empty Inprog, exiting..\n");
        exit(EXIT_FAILURE);
    }
    if(reqptr->inprog->atomic_counter == inprog.atomic_counter && reqptr->next == NULL) {
        // only 1 element in list (head), reset it but don't free the global head!
        request_ll_free(&reqptr->inprog->req_ll_head);
        free(reqptr->inprog->inprog_lock);
        free(reqptr->inprog->complete_lock);
        pthread_cond_destroy(reqptr->inprog->complete_cond);
        free(reqptr->inprog);
        reqptr = NULL;
        *head = NULL;
        return 0;
    }
    while(reqptr != NULL) {
        if(reqptr->next != NULL) {
            if(reqptr->next->inprog->atomic_counter == inprog.atomic_counter) {
                tmp = reqptr->next->next;
                request_ll_free(&reqptr->next->inprog->req_ll_head);
                free(reqptr->next->inprog->inprog_lock);
                free(reqptr->next->inprog->complete_lock);
                pthread_cond_destroy(reqptr->next->inprog->complete_cond);
                free(reqptr->next->inprog);
                free(reqptr->next);
                reqptr->next = tmp;
            }
        }
        reqptr = reqptr->next;
    }
    return 0;
}
