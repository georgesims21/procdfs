#ifndef PROCSYS_QUEUE_H
#define PROCSYS_QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct queue {
    struct queue *next;
    struct bufelem *node;
}QUEUE;

typedef struct bufelem {
    char *buf;
    char complete;
}BUFELEM;


/* Enqueue a NODE into the queue
 * @return
 * 0 upon success
 * -1 upen failiure
 */
int enqueue(QUEUE **queue, BUFELEM *bufelem);

/* Dequeue a NODE from the queue
 * @return
 * NODE located at front of the queue
 */
BUFELEM *dequeue(QUEUE **queue);

/* Print queue contents */
void printq(QUEUE **queue);

/* Deallocate (free) all heap memory in queue
 * @return
 * 0 upon success
 * -1 upon failiure
 */
int dealloq(QUEUE **queue);

/* Return the length of the queue
 * @return
 * int containing length of queue
 */
int lenq(QUEUE **queue);

#endif //PROCSYS_QUEUE_H
