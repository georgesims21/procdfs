#include "client-server.h"
#include "server.h"
#include "log.h"

/*
 * TODO
 *  - [x] Basic client-server model
 *  - [x] Hold more than 1 connection (with comms.)
 *  - [x] Ping messages from server to all clients (notification)
 *  - [x] Organise into functions
 *  - [ ] maxsd isn't used after is is assigned
 *  - [ ] Make the server use one of the procsys filesystems as a client
 *      - [ ] Make a read and write process for the server to communicate with fs
 */

struct sockaddr_in server_addr;

int init_server(int queue_len, struct sockaddr_in *server_add) {
    int opt = 1, server_sock = 0;

    timestamp_log("server");

    lprintf("{server}Creating TCP stream socket\n");
    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    lprintf("{server}filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    lprintf("{server}Binding socket to server address\n");
    if(bind(server_sock, (struct sockaddr*)server_add, sizeof(*server_add)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    lprintf("{server}server is listening on port: %d\n", SERVER_PORT);
    printf("Server initiated successfully\n");
    listen(server_sock, queue_len);
    return server_sock;
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

void disconnect_sock(int socket_set[], struct sockaddr_in *server_add, int sd, int len, int arrpos) {
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
            continue;
        send(socket_set[j], line, strlen(line), 0);
    }
}

void write_loop(int sock) {
    char line[MAX] = {0};
    while(1) {
        bzero(line, MAX);
        fgets(line, MAX, stdin);
        line[strlen(line) - 1] = 0;
        if(line[0] == 0) {
            lprintf("{server}Nothing entered exiting..\n");
            exit(0);
        }
        if((write(sock, line, MAX)) < 0) {
            perror("write");
            exit(1);
        }
        lprintf("{server}Sent message: %s\n", line);
        if(strcmp(line, "exit") == 0)
            exit(0);
    }
}

int listenfds(int socket_set[], int server_sock, fd_set *fds, int sd) {
    int maxsd = 0;
    FD_ZERO(fds);
    FD_SET(server_sock, fds);
    maxsd = add_clients(socket_set, sd, server_sock, fds);
    if((select( MAX_CLIENTS + 1, fds, NULL,
                NULL, NULL)) < 0) { // indefinitely (timeout = NULL)
        perror("select");
        exit(EXIT_FAILURE);
    }
    return maxsd;
}

void accept_connection(int socket_set[], int server_sock, int new_sock, int len, char *message,
        struct sockaddr_in *server_add) {
    if ((new_sock = accept(server_sock,
                           (struct sockaddr *)server_add, (socklen_t *)&len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    lprintf("{server}New connection , socket fd is %d , ip is : %s , port : %d\n", CONN_MSG, new_sock,
           inet_ntoa(server_add->sin_addr), ntohs(server_add->sin_port));

    if(send(new_sock, message, strlen(message), 0) != strlen(message)) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    if(add_socket(socket_set, new_sock) < 0)
        lprintf("{server}socket fd array full!");
}

char handle_client(int socket_set[], int sd, int len, int i, char *line, struct sockaddr_in *server_add) {
    /*
     * TODO
     *  - [ ] Handle the line[MAX-1] hack
     */
    if ((read(sd, line, MAX)) == 0) {
        //Somebody disconnected
        disconnect_sock(socket_set, server_add, sd, len, i);
        return 1;
    }
    // Client message
    line[MAX - 1] = '\0';
    if(strcmp(line, "exit") == 0) {
        disconnect_sock(socket_set, server_add, sd, len, i);
        return 1;
    }
    return 0;
}

void server_loop(int server_sock, int len, char *message) {
    int client_socks[MAX_CLIENTS] = {0}, new_sock = 0, sd = 0, maxsd = 0;
    unsigned int i = 0;
    char line[MAX] = {0};
    fd_set fdset;

    while(1) { // for accepting connections
        maxsd = listenfds(client_socks, server_sock, &fdset, sd);
        //If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(server_sock, &fdset)) {
            accept_connection(client_socks, server_sock, new_sock, len, message, &server_addr);
        }
        //else its some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socks[i];
            if (FD_ISSET(sd , &fdset)) {
                //Check if it was for closing , and also read the incoming message
                if(handle_client(client_socks, sd, len, i, line, &server_addr) == 0)
                    // notify all other connected clients about message
                    notify_clients(client_socks, sd, line);
            }
        }
    }
}

void run_server(void) {
    int server_sock = 0, len = 0, pid = 0;
    char message[MAX] = {0};

    server_sock = init_server(10, &server_addr);
    len = sizeof(server_addr);
    sprintf(message, "{server}%dConnected to server address at %s and port %hu...",
            CONN_MSG, inet_ntoa(server_addr.sin_addr) , ntohs(server_addr.sin_port));

    server_loop(server_sock, len, message);
}

int main(int argc, char *argv[]) {
    run_server();
    return 0;
}

