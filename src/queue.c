#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int enqueue(QUEUE **queue, NODE *node) {
/*
 * TODO
 *   * Error check the malloc with errno
 */
    QUEUE *qp = queue;
    QUEUE *new_elem = (QUEUE *)malloc(sizeof(QUEUE));
    if(!new_elem) {
        // unsuccessful malloc
        return -1;
    }
    new_elem->node = node;
    if(!qp)
        // Empty queue
        *queue = new_elem;
    else {
        while(qp->next) {
            // Use qp as queue pointer to traverse elems
            qp = qp->next;
        }
        qp->next = new_elem;
    }
    new_elem->next = NULL;
    return 0;
}

NODE dequeue(QUEUE **queue) {
/*
 * TODO
 *   * Need to error check when queue is empty
 */
    QUEUE *qp = *queue;
    if(!qp) {
        // Empty queue
        printf("queue already empty!");
    } else {
        NODE *elem = qp->node;
        free(qp->node);
        if(qp->next) {
            *queue = qp->next;
        } else {
            *queue = NULL;
        }
        printf("Dequeued NODE: %s\n", elem->name);
        return *elem;
    }
}
void printq(QUEUE **queue) {

    QUEUE *qp = *queue;
    int count = 0;
    if(!qp)
        printf("Empty queue!\n");
    else {
        while (qp) {
            printf("queue[%d]:\t%s\n", count, qp->node->name);
            qp = qp->next;
            count++;
        }
    }
}

int dealloq(QUEUE **queue) {

    QUEUE *qp = *queue;
    if(!qp)
        return 0;
    else {
        while (qp) {
            // Assumes all nodes are allocated on the heap
            free(qp->node);
            qp = qp->next;
        }
    }
    printf("Dealloq'ed..");
}

int main(int argc, char *argv[]) {
/*
 * TODO
 */
    QUEUE *test = {NULL};
    printq(&test);
    NODE *nn = (NODE *)malloc(sizeof(NODE));
    NODE *n2 = (NODE *)malloc(sizeof(NODE));
    strcpy(nn->name,"testing");
    strcpy(n2->name, "Another one");
    enqueue(&test, nn);
    enqueue(&test, n2);
    printq(&test);

    dequeue(&test);
    printq(&test);

    dealloq(&test);

    return 0;
}
