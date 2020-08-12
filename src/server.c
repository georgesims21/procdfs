#include "client-server.h"
#include "server.h"
#include "log.h"
#include "fileops.h"
#include "reader.h"
#include "writer.h"
#include "defs.h"
#include "queue.h"

/*
 * TODO
 *  * writer
 *  * reader
 *  * main
 *  - [ ] maxsd isn't used after is is assigned
 *  - [ ] send buf to all clients, skip fd of caller
 *  - [x] struct containing calling clients fd with flag, path and content
 *  - [ ] queue data struct to hold received buffers
 *      - queue empty method: keep queue length as global and constantly check in while loop
 *  - [ ] method to congregate values from all the buffers
 *      - [ ] make list of files containing static information - don't congregate the rest yes?
 */

CALLER caller = {0};
QUEUE *queue = {0};

size_t fdsetlen(int socket_set[]) {
    size_t count = 0;
    for(unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if(socket_set[i] != 0)
            count++;
    }
    return count;
}

int init_server(int queue_len, struct sockaddr_in *server_add) {
    int opt = 1, socket_fd, r;
    struct sockaddr_in *tmp = {0};

    timestamp_log("server");

    lprintf("{server}Creating TCP stream socket\n");
    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    lprintf("{server}filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    lprintf("{server}Binding socket to server address\n");
    if(bind(socket_fd, (struct sockaddr*)server_add, sizeof(*server_add)) < 0) {
        if (errno == EADDRINUSE) {
            lprintf("{server %d}address already in use, returning...\n", getpid());
            return -1;
        } else {
            perror("bind");
            exit(EXIT_FAILURE);
        }
    }

    lprintf("{server}server is listening on port: %d\n", SERVER_PORT);
    listen(socket_fd, queue_len);
    return socket_fd;
}

int add_clients(int socket_set[], int sd, int maxsd, fd_set *newfdset) {
    for(unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if((sd = socket_set[i]) > 0) {
            FD_SET(sd, newfdset); // if valid add to set
        }
        if(sd > maxsd) {
            maxsd = sd;
        }
    }
    return maxsd;
}

int add_socket(int socket_set[], int new_sock) {
    //add new socket to array of sockets
    for(unsigned int i = 0; i < MAX_CLIENTS; i++) {
        //if position is empty
        if(socket_set[i] == 0) {
            socket_set[i] = new_sock;
            lprintf("{server}Adding to list of sockets as %d\n" , i);
            return 0;
        }
    }
    return -1; // array full
}

void disconnect_sock(int socket_set[], struct sockaddr_in *server_add, int sd, int len, unsigned int arrpos) {
    /*
     * TODO
     *  - [ ] error checking for close method
     */

    // to be used in a loop
    getpeername(sd, (struct sockaddr*)server_add , (socklen_t *)&len);
    lprintf("{server}Host disconnected , ip %s , port %d \n",
           inet_ntoa(server_add->sin_addr), ntohs(server_add->sin_port));

    //Close the socket and mark as 0 in list for reuse
    close(sd);
    socket_set[arrpos] = 0;
}

void notify_clients(int socket_set[], int sd, char *line) {
    for(unsigned int j = 0; j < MAX_CLIENTS; j++) {
        if(sd == socket_set[j])
            send(socket_set[j], line, strlen(line), 0);
    }
}

int listenfds(int socket_set[], int server_socket, fd_set *fds, int sd) {
    int maxsd = 0;
    FD_ZERO(fds);
    FD_SET(server_socket, fds);
    maxsd = add_clients(socket_set, sd, server_socket, fds);
    if((select( MAX_CLIENTS + 1, fds, NULL,
                NULL, NULL)) < 0) { // indefinitely (timeout = NULL)
        perror("select");
        exit(EXIT_FAILURE);
    }
    return maxsd;
}

void accept_connection(int socket_set[], int server_socket, int new_sock, int len,
        struct sockaddr_in *server_add) {

    char message[MAX] = {0};

    if ((new_sock = accept(server_socket,
                           (struct sockaddr *)server_add, (socklen_t *)&len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    lprintf("{server}New connection , socket fd is %d , ip is : %s , port : %d\n", new_sock,
           inet_ntoa(server_add->sin_addr), ntohs(server_add->sin_port));

    sprintf(message, "%d%dConnected to server address at %s and port %hu...", CONN_MSG_SER, new_sock,
            inet_ntoa(server_add->sin_addr) , ntohs(server_add->sin_port));

    if(send(new_sock, message, strlen(message), 0) != strlen(message)) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    if(add_socket(socket_set, new_sock) < 0)
        lprintf("{server}socket fd array full!");
}

int handle_client(int socket_set[], int sd, int len, unsigned int i, char *line, struct sockaddr_in *server_add) {

    char tmp[MAX] = {0};
    char path[64] = {0};
    int pid = 0;
    BUFELEM elem = {0};
    BUFELEM *extracted = {0};

    if ((read(sd, line, READ_MAX)) == 0) {
        //Somebody disconnected
        return CLI_DISCONNECT;
    }
    // Client message
//    line[TERM_CHAR_MAX] = '\0';

    switch(parse_flag(line)) {
        case NME_MSG_CLI:
            caller.fd = sd;
            snprintf(caller.path, MAX_PATH, "%s", line);
            memset(line, 0, sizeof(line));
            lprintf("{server}[file request]for path \"%s\" received from client(sd){%d}\n",
                    caller.path, caller.fd);
            snprintf(line, strlen(caller.path) + 1, "%s", caller.path);
            return CLI_SEND_ALL;
        case CNT_MSG_CLI:
            lprintf("{server}[file content]for path \"%s\" received from client(sd){%d}\n",
                    caller.path, sd, line);
            // we only have 1 server process: can we use last one to do queue work
            if(caller.fd == sd) {
                lprintf("{server} caller.fd == sd\n");
                snprintf(caller.content, strlen(line) + 1, "%s", line);
            } else {
                lprintf("{server} caller.fd != sd\n");
                snprintf(elem.buf, strlen(line) + 1, "%s", line);
                elem.complete = 0;
                enqueue(&queue, &elem);
            }
//            lprintf("{server} queue len = %d\nfdsetlen = %d\n", lenq(&queue), fdsetlen(socket_set));
            if(lenq(&queue) == fdsetlen(socket_set) - 1) {
                // if this loop is the final one for the server, all files must have been received.
                // Possibility n didn't arrive but problem for the thesis paper. Can't have race cond b/c of
                // having only 1 process and the fdset. Never reads 2 at once from the set. Think about if
                // 2 fs' call a read at the same time.
                lprintf("{server} entered conditional queuelen == socketlen\n");
                while(lenq(&queue) > 0) {
                    lprintf("{server} entered while loop\n");
                    dequeue(&queue);
                    lprintf("{server} dequeued sucessfully\n");
                    // concat with caller.content
                }
                return CLI_SEND_CALLER;
            }
            return END_PROC_SER;
        default:
            break;
    }
    return 0;
}

void server_loop(int server_socket, int len, struct sockaddr_in *server_addr) {
    int client_socks[MAX_CLIENTS] = {0}, new_sock = 0, sd = 0, maxsd = 0;
    unsigned int i = 0;
    char line[MAX] = {0};
    fd_set fdset;


    while(1) { // for accepting connections
        maxsd = listenfds(client_socks, server_socket, &fdset, sd);
        //If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(server_socket, &fdset)) {
            accept_connection(client_socks, server_socket, new_sock, len, server_addr);
        }
        //else its some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socks[i];
            if (FD_ISSET(sd , &fdset)) {
                //Check if it was for closing , and also read the incoming message
                switch (handle_client(client_socks, sd, len, i, line, server_addr)) {
                    case CLI_DISCONNECT:
                        disconnect_sock(client_socks, server_addr, sd, len, i);
                        break;
                    case CLI_SEND_ALL:
                        // requesting the file from all clients but the caller
                        prepend_flag(REQ_MSG_SER, line);
                        for(unsigned int j = 0; j < MAX_CLIENTS; j++) {
//                            if(caller.fd == client_socks[j])
//                                continue;
                            send(client_socks[j], line, strlen(line), 0);
                        }
                        break;
                    case CLI_SEND_CALLER:
                        // send final file to caller
                        // problem, this will be called n times for amount of connected clients
                        // if
                        prepend_flag(FIN_MSG_SER, caller.content);
                        send(caller.fd, caller.content, strlen(caller.content), 0);
                        memset(&caller, 0, sizeof(caller));
                        break;
                    case END_PROC_SER:
                        // want this process to go back to start of while to listen for more files
                        lprintf("{server} END_PROC_SER hit\n");
                        break;
                    default:
                        break;
                }
                memset(line, 0, sizeof(line));
            }
        }
    }
}
//
//void run_server(void) {
//    int len = 0;
//
//    server_sock = init_server(10, &server_addr);
//    len = sizeof(server_addr);
//
//
//    server_loop(server_sock, len, message);
//}
//
//int main(int argc, char *argv[]) {
//    run_server();
//    return 0;
//}

