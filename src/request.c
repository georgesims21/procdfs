#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//#include "inprog.h"
#include "request.h"
#include "server.h"

static int request_cmp(Request req1, Request req2) {

    if (req1.atomic_counter == req2.atomic_counter &&
        strncmp(req1.path, req2.path, strlen(req1.path)) == 0 &&
        req1.sender.addr.sin_addr.s_addr == req2.sender.addr.sin_addr.s_addr) {
        return 0;
    }
    return 1;
}

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
    int fd = -1;
    fd = openat(-100, path, O_RDONLY);
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

    /*
     * https://en.wikipedia.org/wiki/Joel_Spolsky#Schlemiel_the_Painter's_algorithm
     */
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
    int fd = -1, res = 0, offset = 0, size = 0;
    fd = openat(-100, path, O_RDONLY);
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
