#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int enqueue(QUEUE **queue, NODE *node) {
/*
 * TODO
 *   * Error check the malloc
 */
    QUEUE *local_q = *queue;
    QUEUE *new_elem = (QUEUE *)malloc(sizeof(QUEUE));
    new_elem->node = node;
    if(local_q == NULL)
        *local_q = *new_elem;
    else {
        while(local_q->next) {
            local_q = local_q->next;
        }
        local_q->next = new_elem;
    }
    new_elem->next = NULL;
    return 0;
}
void printq(QUEUE **queue) {

    QUEUE *local_q = *queue;
    int count = 0;
    if(local_q == NULL)
        printf("Empty queue!");
    else {
        while(local_q->next)
            printf("queue[%d]:\t%s\n", count, local_q->node->name);
            local_q = local_q->next;
            count++;
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
    NODE *nn = {NULL};
    enqueue(&test, nn);
//    printq(&test);
    return 0;
}
