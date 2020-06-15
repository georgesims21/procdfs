#include "queue.h"

int enqueue(QUEUE **queue, NODE *node) {
/*
 * TODO
 *   * Error check the malloc with errno
 */
    QUEUE *qp = *queue;
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
    printf("Enqueue'd node: \"%s\"\n", new_elem->node->name);
    return 0;
}

NODE dequeue(QUEUE **queue) {
/*
 * TODO
 *   * Need to error check when queue is empty
 */
    QUEUE *qp = *queue;
    NODE *elem = NULL;
    if(!qp) {
        // Empty queue
        printf("queue already empty!");
    } else {
        elem = qp->node;
        free(qp->node);
        if(qp->next) {
            *queue = qp->next;
        } else {
            *queue = NULL;
        }
        printf("Dequeued NODE: %s\n", elem->name);
    }
    return *elem;
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
    if(!qp) {
        printf("Queue is empty, not dealloq'ing");
        return -1;
    }
    else {
        while (qp) {
            // Assumes all nodes are allocated on the heap
            free(qp->node);
            qp = qp->next;
        }
    }
    printf("Dealloq'ed..");
    return 0;
}

int lenq(QUEUE **queue) {
    int count = 0;
    QUEUE *qp = *queue;
    while(qp) {
        count++;
        qp = qp->next;
    }
    return count;
}

//int main(int argc, char *argv[]) {
///*
// * TODO
// */
//    QUEUE *test = {NULL};
//    printq(&test);
//    NODE *nn = (NODE *)malloc(sizeof(NODE));
//    NODE *n3 = (NODE *)malloc(sizeof(NODE));
//    strcpy(nn->name,"testing");
//    strcpy(n3->name, "Another one");
//    enqueue(&test, nn);
//    enqueue(&test, n3);
//    printq(&test);
//
//    printf("queue length = %d\n", lenq(&test));
//
//    dequeue(&test);
//    printq(&test);
//
//    NODE *n4 = (NODE *)malloc(sizeof(NODE));
//    NODE *n5 = (NODE *)malloc(sizeof(NODE));
//    strcpy(n4->name,"third");
//    strcpy(n5->name, "fourth");
//    enqueue(&test, n4);
//    enqueue(&test, n5);
//    printq(&test);
//    printf("queue length = %d\n", lenq(&test));
//
//    dealloq(&test);
//
//    return 1;
//}
