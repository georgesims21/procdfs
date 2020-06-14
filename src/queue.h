#ifndef PROCSYS_QUEUE_H
#define PROCSYS_QUEUE_H

#include "tree.h"

typedef struct queue {
    struct queue *next;
    struct node *node;
} QUEUE;

/* Push a tree NODE into the queue
 * @return
 * 0 upon success
 * -1 upen failiure
 */
int enqueue(QUEUE **queue, NODE *node);

#endif //PROCSYS_QUEUE_H
