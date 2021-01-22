#ifndef PROCSYS_DS_NEW_H
#define PROCSYS_DS_NEW_H

#include <glob.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>

#include "request.h"

/*
 * "In-progress" request. Holds the grouped request linked list together and allows them to be searchable on
 * the atomic counter. Once all requests in this group are marked as complete, this Inprog can also be marked
 * as complete. Also it holds two locks and a conditional variable for concurrent access to itself and the
 * variable which marks it as complete (as there is contention on this).
 */
typedef struct inprog {
    /*
     * Atomic counter the requests are grouped on
     */
    long long atomic_counter;
    /*
     * Total number of sent requests for this atomic counter
     */
    int messages_sent;
    /*
     * If all requests in the inner list are marked as complete, this is true. Else false
     */
    bool complete;
    /*
     * Mutex lock for this structure
     */
    pthread_mutex_t *inprog_lock;
    /*
     * Mutex lock for the
     */
    pthread_mutex_t *complete_lock;
    /*
     * Allow concurrent access to the complete boolean
     */
    pthread_cond_t *complete_cond;
    /*
     * The head of the inner linked list containing the grouped requests
     */
    Request_tracker_node *req_ll_head;
}Inprog;

/*
 * Allows for "in-progress" requests (Inprogs) to be grouped together in a linked list.
 */
typedef struct inprog_tracker_node {
    /*
     * Contains the data about the single "in-progress" request
     */
    Inprog *inprog;
    /*
     * Pointer to the next inprog_tracker_node in the linked list
     */
    struct inprog_tracker_node *next;
}Inprog_tracker_node;

/*
 * Total number of connected machines
 */
extern int nrmachines;
/*
 * Atomic counter
 */
extern long long a_counter;
/*
 * Address information of the host machine
 */
extern Address host_addr;
/*
 * Array containing all connected clients
 */
extern Address *connected_clients;
/*
 * Head of the 'outer' linked list containing all "in-progress" requests
 */
extern Inprog_tracker_node *inprog_tracker_head;
/*
 * Mutex lock of the connected clients array
 */
extern pthread_mutex_t connected_clients_lock;
/*
 * Mutex lock of the "in-progress" Inprog linked list
 */
extern pthread_mutex_t inprog_tracker_lock;
/*
 * Mutex lock of the atomic counter a_counter variable
 */
extern pthread_mutex_t a_counter_lock;



void inprog_free(Inprog *inp);

///*
// * Frees the items located inside of this request linked list from the heap. This is for benchmarking
// * purposes only.
// *
// * @param head head of the linked list
// */
//int request_ll_free(Request_tracker_node **head);

void inprog_reset(Inprog *inp);

///*
// * Counts how many requests are complete within a given linked list.
// *
// * @param head head of the linked list
// * @return the count
// */
//int request_ll_complete(Request_tracker_node **head);

/*
 * Add file content to a request located within a given Inprog. This method also checks to see if all requests
 * have been complete within the list, and marks the Inprog as complete if this is true before broadcasting to the
 * given inprogs conditional variable.
 *
 * @param req the request to add the file content to
 * @param head head of the linked list
 * @param inprog_lock the lock of the Inprog being queried
 * @return 0 upon success, exit on failure
 */
int inprog_add_buf(Request *req, Inprog *inprog, pthread_mutex_t *inprog_lock);


/*
 * Add a request_tracker_node to an 'outer' Inprog linked list.
 *
 * @param head the head of the linked list
 * @param inprog the Inprog to be added to the list
 * @return -1 on error and 0 on success
 */
int inprog_tracker_ll_add(Inprog_tracker_node **head, Inprog *inprog);

/*
 * Print the Inprog tracker linked list.
 *
 * @param head head of the linked list
 */
void inprog_tracker_ll_print(Inprog_tracker_node **head);

/*
 * Fetch an Inprog tracker linked list node given a request using the atomic counter.
 *
 * @param head head of the linked list
 * @param req the request containing the atomic counter to be searched
 */
Inprog_tracker_node *inprog_tracker_ll_fetch_req(Inprog_tracker_node **head, Request req);

/*
 * Fetch an Inprog tracker linked list node given a request using the atomic counter.
 *
 * @param head head of the linked list
 * @param req the request containing the atomic counter to be searched
 */
Inprog_tracker_node *inprog_tracker_ll_fetch_node(Inprog_tracker_node **head, Inprog inprog);

/*
 * Remove a Inprog_tracker_node from the 'outer' linked list given the Inprog.
 *
 * @param head head of the linked list
 * @param inprog the Inprog to be removed
 */
int inprog_tracker_ll_remove(Inprog_tracker_node **head, Inprog inprog);

//int request_ll_countbuflen(Request_tracker_node **head);
//char *request_ll_catbuf(Request_tracker_node **head);

#endif //PROCSYS_DS_NEW_H
