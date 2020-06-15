#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int enqueue(QUEUE **queue, NODE *node) {
/*
 * TODO
 *   * Error check the malloc with errno
 */
    QUEUE *qp = *queue;
    QUEUE *new_elem = (QUEUE *)malloc(sizeof(QUEUE));
    if(new_elem == NULL) {
        // unsuccessful malloc
        return -1;
    }
    new_elem->node = node;
    if(*queue == NULL)
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
void printq(QUEUE **queue) {

    QUEUE *local_q = *queue;
    int count = 0;
    if(local_q == NULL)
        printf("Empty queue!\n");
    else {
        do {
            printf("queue[%d]:\t%s\n", count, local_q->node->name);
            local_q = local_q->next;
            count++;
        } while(local_q->next);
    }
}

int main(int argc, char *argv[]) {
/*
 * TODO
 *   * Getting seg fault when using the enqueue, print not fully tested either
 */
    QUEUE *test = {NULL};
//    memset(test, 0, sizeof(struct queue));
    printq(&test);
    NODE *nn;
    strcpy(nn->name, "testing");
    enqueue(&test, nn);
    printq(&test);
    return 0;
}
