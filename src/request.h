#ifndef PROCDFS_REQUEST_H
#define PROCDFS_REQUEST_H

#define MAXPATH 64
/*
 * Contains all information needed for sending/receiving/identifying a machine.
 */
typedef struct address {
    /*
     * Socket for receiving
     */
    int sock_in;
    /*
     * Socket for sending
     */
    int sock_out;
    /*
     * Length of the addr struct
     */
    size_t addr_len;
    /*
     * Containing IP and port specific information
     */
    struct sockaddr_in addr;
}Address;

/*
 * Contains all information about a single request including sender address information and an atomic
 * counter to be able to group multiple requests together. Also this includes a flexible array member (buf)
 * to allow the struct to be realloc'ed when the file content is received, and thus dynamically adding file
 * content to the structure.
 */
typedef struct request {
    /*
     * Address information about where this request is being sent to
     */
    Address sender;
    /*
     * Atomic counter to define and group requests
     */
    long long atomic_counter;
    /*
     * Allows request to be marked as complete when the file content is added (and this request is satisfied)
     */
    bool complete;
    /*
     * Path of the file linked to this request (and atomic counter)
     */
    char path[MAXPATH];
    /*
     * Length of the file content
     */
    unsigned long buflen;
    /*
     * File content (flexible array member)
     */
    char buf[];
}Request;

/*
 * Allows for requests to be grouped together in a linked list.
 */
typedef struct request_tracker_node {
    /*
     * The actual request
     */
    Request *req;
    /*
     * Pointer to the next request_tracker_node in the linked list
     */
    struct request_tracker_node *next;
}Request_tracker_node;

/*
 * Add a request_tracker_node to an 'inner' request linked list.
 *
 * @param head the head of the linked list
 * @param req the request to be added to the list
 * @return -1 on error and 0 on success
 */
int req_tracker_ll_add(Request_tracker_node **head, Request *req);

/*
 * Print the request tracker linked list.
 *
 * @param head head of the linked list
 */
void req_tracker_ll_print(Request_tracker_node **head);

/*
 * Add the file content to an already existing node in a request linked list.
 *
 * @param head head of the linked list
 * @param req the request to be added to the list
 * @param cnt the file content
 * @param cntlen the size of the file content buffer
 */
int req_add_content(Request_tracker_node **head, Request req, char *cnt, int cntlen);


/*
 * Fetch a request tracker linked list node given a request.
 *
 * @param head head of the linked list
 * @param req the request to be searched in the list
 */
Request_tracker_node *req_tracker_ll_fetch(Request_tracker_node **head, Request req);


/*
 * Frees the items located inside of this request linked list from the heap. This is for benchmarking
 * purposes only.
 *
 * @param head head of the linked list
 */
int request_ll_free(Request_tracker_node **head);

/*
 * Counts how many requests are complete within a given linked list.
 *
 * @param head head of the linked list
 * @return the count
 */
int request_ll_complete(Request_tracker_node **head);

/*
 * Count the length of the final file given the head of the Request_tracker_node linked list. Once the Inprog
 * is complete, the file contents are located within their corresponding request structure. This method
 * goes through each of these and accumalates the total size of the buffers added together. It then counts it's
 * own machine's matching procfs file and adds that to the total. The return value is the final size of the
 * file which will be given back to the calling process.
 *
 * @param head head of the linked list
 * @return total size of the buflens of each file in the request linked list put together
 */
int request_ll_countbuflen(Request_tracker_node **head);

/*
 * Collect all file buffers from within a single linked list and concatenate them together along with the current
 * host machine's corresponding procfs file, and return that final buffer.
 *
 * @param head head of the linked list
 * @return the final file buf of all machine's files concatenated together
 */
char *request_ll_catbuf(Request_tracker_node **head);

/*
 * Compare two requests to see if they are equal.
 *
 * @param req1 the first request to be compared
 * @param req2 the second request to be compared
 * @return 0 if equal, 1 if they are not
 */
static int request_cmp(Request req1, Request req2);

#endif //PROCDFS_REQUEST_H
