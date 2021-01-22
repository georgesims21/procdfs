#ifndef PROCSYS_SERVER_NEW_H
#define PROCSYS_SERVER_NEW_H

#include <glob.h>
#include <netinet/in.h>

#include "inprog.h"

/*
 * Flags used in the message headers.
 * FREQ - File request from a different machine
 * FCNT - Reply to a request, contains that machine's file content
 */
#define FREQ 1
#define FCNT 2
/*
 * Total length of the header used in the messaging protocol.
 * 32 (bytes) - Host IP
 * 16 - Host port
 * 32 - Recepient IP
 * 16 - Recepient Port
 * 64 - Atomic counter (long long)
 * MAXPATH - Max path length
 * 8 - Terminating char and delimiter chars
 */
#define HEADER 166

/*
 * Used for consistent handling of memory allocation errors
 */
void malloc_error(void);
void realloc_error(void);
void calloc_error(void);

/*
 * Initialize the server. This includes setting up the listening socket using the given params.
 *
 *  @param address information about host
 *  @param queue_length define max queue length for the listening socket
 *  @param port_number port number to use for listening socket
 *  @param interface interface to be used
 *  @param ip ip address
 *  @return 0 if successful, program will exit otherwise
 */
int init_server(Address *address, int queue_length, int port_number, const char *interface, const char *ip);

/*
 * Struct given to the connect_to_file_IPs method
 */
struct connect_to_file_IPs_args {
    /*
     * Array containing addresses of the connected clients
     */
    Address *conn_clients;
    /*
     * Mutex lock for the conn_clients array
     */
    pthread_mutex_t *conn_clients_lock;
    /*
     * Address information about the host
     */
    Address host_addr;
    /*
     * Length of the conn_clients array (total machines -1)
     */
    int arrlen;
    /*
     * Name of the IP file to be used
     */
    const char *filename;
};

/*
 * Connect to all of the IP addresses on the given IP list. This method will parse a line, connect to this IP
 * and if successful it will store the socket and the address information into the conn_clients array. If
 * connection wasn't possible it will skip and try again later. This method returns when it has connected to all
 * (except its own) IP addresses on the list.
 *
 *  @param connect_to_file_IPs_args
 */
void *connect_to_file_IPs(void *arg);

/*
 * Struct given to the connect_to_file_IPs method
 */
struct accept_connection_args {
    /*
     * Array containing addresses of the connected clients
     */
    Address *conn_clients;
    /*
     * Mutex lock for the conn_clients array
     */
    pthread_mutex_t *conn_clients_lock;
    /*
     * Address information about the host
     */
    Address host_addr;
    /*
     * Length of the conn_clients array (total machines -1)
     */
    int arrlen;
    /*
     * Name of the IP file to be used
     */
    const char *filename;
};

/*
 * Accept incoming connections via the host listening socket. A new thread is created each time so the main thread
 * can go back to listening on the socket.
 *
 * @param accept_connection_args
 */
void *accept_connection(void *arg);

/*
 * Struct given to the server_loop method
 */
struct server_loop_args {
    /*
     * Array containing addresses of the connected clients
     */
    Address *conn_clients;
    /*
     * Mutex lock for the conn_clients array
     */
    pthread_mutex_t *conn_clients_lock;
    /*
     * Address information about the host
     */
    Address host_addr;
    /*
     * Length of the conn_clients array (total machines -1)
     */
    int arrlen;
    /*
     * Name of the IP file to be used
     */
    const char *filename;
};

/*
 * The main workhorse of the server code. This is an infinite loop which monitors for incoming connections and
 * messages. This method parses the data from incoming messages, first extracting the header information and storing
 * it. Then depending on the flag the server_loop will:
 * FREQ - Another machine requesting a file from this one. Collect the file data into a buffer and return this
 * buffer with a prepended header.
 * FCNT - This machine is receiving a reply from a request. Collect the content, search for the corresponding request
 * using the header metadata, then store the file content there. Once all requests have been returned the conditional
 * variable is set, notifying the waiting file system thread.
 *
 * @param server_loop_args
 */
void *server_loop(void *arg);

struct new_connection_args {
    /*
     * Array containing addresses of the connected clients
     */
    Address *conn_clients;
    /*
     * Mutex lock for the conn_clients array
     */
    pthread_mutex_t *conn_clients_lock;
    /*
     * Length of the conn_clients array (total machines -1)
     */
    Address host_addr;
    /*
     * Length of the conn_clients array (total machines -1)
     */
    int arrlen;
    /*
     * Name of the IP file to be used
     */
    const char *filename;
};

/*
 * Given address information, create a message with the correct header information. Append the file content
 * only if the FCNT is used.
 *
 * @param host_addr address information of the host machine
 * @param req the request containing info about the recipient machine
 * @param flag give either FREQ or FCNT flag to message header
 * @return the created message string
 */
char *create_message(Address host_addr, Request *req, int headerlen, int flag);

/*
 * Create an "in-progress" request (Inprog). This method creates and allocates all variables and datastructures
 * needed for the Inprog. It then creates requests based on the given file and sends them to the machines
 * in the connected_clients array. The returned Inprog structure is now active and replies will be sent by other
 * machines after it is returned.
 *
 * @param path the path of the file to be requested
 * @return the created Inprog
 */
Inprog *inprog_create(const char *path);

/*
 * Create the Inprog first, then add it to the outer "in-progress" linked list and wait on the conditional variable.
 * This is used by the file system thread to wait for an Inprog to be fulfilled, which then in turn can use this
 * data within the file system (like concatenating the file contents from each machine into one file).
 *
 * @param path the path of the file to be requested
 * @return the created Inprog
 */
Inprog *file_request(const char *path);

/*
 * Return the size of a file, given a file descriptor. The bytes on the files are counted individually due to
 * procfs files being 0 in size.
 *
 * @param file descriptor of the file
 * @return the size of the file, otherwise exit failiure
 */
int procsizefd(int fd);

#endif //PROCSYS_SERVER_NEW_H
