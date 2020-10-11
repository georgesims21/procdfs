#include "ds_new.h"
#include "log.h"
#include "server_new.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

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
        next = reqptr->next;
        if(reqptr->req != NULL) {
            free(reqptr->req);
        }
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

int request_ll_countbuflen(Request_tracker_node **head) {

    Request_tracker_node *reqptr = *head;
    Request_tracker_node *next;
    int count = 0;
    if(reqptr == NULL) {
        printf("The list is empty!\n");
        return 0;
    }
    char *path = reqptr->req->path;
    while(reqptr != NULL) {
        if(reqptr->req != NULL) {
            if(reqptr->req->complete) {
                count += reqptr->req->buflen;
            } else {
                printf("Request not complete, exiting...\n");
                exit(EXIT_FAILURE);
            }
        }
        next = reqptr->next;
        reqptr = next;
    }
    int fd = -1, res = 0, offset = 0, size = 0, err = 0;
    fd = openat(AT_FDCWD, path, O_RDONLY);
    if (fd == -1) {
        perror("openat");
        printf("%s\n", path);
        exit (EXIT_FAILURE);
    }
    count += procsizefd(fd);
    close(fd);
    return count;
}

char *request_ll_catbuf(Request_tracker_node **head) {
    // doesn't check filebuf for non-NULL etc

    Request_tracker_node *reqptr = *head;
    Request_tracker_node *next;
    size_t count = 0, old_count = 0;
    char *filebuf = NULL;
    if(reqptr == NULL) {
        printf("The list is empty!\n");
        exit(EXIT_FAILURE);
    }
    char *path = reqptr->req->path;
    while(reqptr != NULL) {
        if(reqptr->req != NULL) {
            if(reqptr->req->complete) {
                old_count = count;
                count += reqptr->req->buflen;
                filebuf = realloc(filebuf, count);
                memset(&filebuf[old_count], 0, reqptr->req->buflen);
                strncat(filebuf, reqptr->req->buf, reqptr->req->buflen);
            } else {
                printf("Request not complete, exiting...\n");
                exit(EXIT_FAILURE);
            }
        }
        next = reqptr->next;
        reqptr = next;
    }
    int fd = -1, res = 0, offset = 0, size = 0, err = 0;
    fd = openat(AT_FDCWD, path, O_RDONLY);
    if (fd == -1) {
        perror("openat");
        printf("%s\n", path);
        exit (EXIT_FAILURE);
    }
    size = procsizefd(fd); // individually count chars in proc file - bottleneck for large fs
    old_count = count;
    count += size;
    char *procbuf = malloc(sizeof(char) * size);
    if(!procbuf){malloc_error();};
    res = pread(fd, procbuf, size, offset);
    if (res == -1) {
        perror("pread");
        exit(EXIT_FAILURE);
    }
    filebuf = realloc(filebuf, count);
    memset(&filebuf[old_count], 0, size);
    strncat(filebuf, procbuf, size);
    free(procbuf);
    close(fd);
    return filebuf;
}

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
        req_tracker_ll_print(&inprog->req_ll_head);
        // check if all requests (inc. this one) have been received
        if(request_ll_complete(&inprog->req_ll_head) == inprog->messages_sent) {
            inprog->complete = true;
            printf("All messages received!\n");
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

int inprog_tracker_ll_add(Inprog_tracker_node **head, Inprog *inprog, pthread_mutex_t *inprog_lock) {

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
    newnode->inprog_lock = inprog_lock;
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
    printf("List empty!\n");
}

Inprog_tracker_node *inprog_tracker_ll_fetch_req(Inprog_tracker_node **head, Request req) {

    if(head == NULL || strcmp(req.path, "") == 0) {
        printf("List is empty/request is empty, exiting..\n");
        exit(EXIT_FAILURE);
    }
    Inprog_tracker_node *listptr = *head;
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
    Inprog_tracker_node *next;
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
        free(reqptr->inprog_lock);
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
                free(reqptr->next->inprog);
                free(reqptr->next);
                reqptr->next = tmp;
            }
        }
        reqptr = reqptr->next;
    }
    return 0;
}
