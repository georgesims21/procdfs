#include "client-server.h"
//#include "server.h"

#define MAX_CLIENTS 10

/*
 * TODO
 *  - [x] Basic client-server model
 *  - [x] Hold more than 1 connection (with comms.)
 *  - [x] Ping messages from server to all clients (notification)
 *  - [ ] Organise into functions
 *      - maxsd isn't used after is is assigned
 *  - [ ] Make a notify setting
 *  - [ ] Make the server use one of the procsys filesystems as a client
 */


int init_server(int queue_len, struct sockaddr_in *server_add) {
    int opt = 1, server_sock = 0;

    printf("Creating TCP stream socket\n");
    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    printf("filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Binding socket to server address\n");
    if(bind(server_sock, (struct sockaddr*)server_add, sizeof(*server_add)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("server is listening on port: %d\n", SERVER_PORT);
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
int add_socket(int client_socks[], int new_sock) {
    //add new socket to array of sockets
    for(unsigned int i = 0; i < MAX_CLIENTS; i++) {
        //if position is empty
        if(client_socks[i] == 0) {
            client_socks[i] = new_sock;
            printf("Adding to list of sockets as %d\n" , i);
            return 0;
        }
    }
    return -1; // array full
}
void disconnect_sock(int client_socks[], struct sockaddr_in server_add, int sd, int len, int arrpos) {
    /*
     * TODO
     *  - [ ] error checking for close method
     */

    // to be used in a loop
    getpeername(sd, (struct sockaddr*)&server_add , (socklen_t *)&len);
    printf("Host disconnected , ip %s , port %d \n",
           inet_ntoa(server_addr.sin_addr), ntohs(server_add.sin_port));

    //Close the socket and mark as 0 in list for reuse
    close(sd);
    client_socks[arrpos] = 0;
}
void notify_clients(int client_socks[], int sd, char *line) {
    for(unsigned int j = 0; j < MAX_CLIENTS; j++) {
        if(sd == client_socks[j])
            continue;
        send(client_socks[j], line, strlen(line), 0);
    }
}


int main(int argc, char *argv[]) {

    int client_socks[MAX_CLIENTS] = {0}, new_sock = 0, sd = 0, maxsd = 0, server_sock = 0, len = 0;
    char line[MAX] = {0}, message[MAX] = {0};
    unsigned int i = 0;
    fd_set fdset;

    server_sock = init_server(10, &server_addr);
    len = sizeof(server_addr);
    sprintf(message, "Connected to server address at %s and port %hu...",
            inet_ntoa(server_addr.sin_addr) , ntohs(server_addr.sin_port));

    while(1) { // for accepting connections
        FD_ZERO(&fdset);
        FD_SET(server_sock, &fdset);
        maxsd = add_clients(client_socks, sd, server_sock, &fdset);
        if((select( MAX_CLIENTS + 1, &fdset, NULL,
                NULL, NULL)) < 0) { // indefinitely (timeout = NULL)
            perror("select");
            exit(EXIT_FAILURE);
        }

        //If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(server_sock, &fdset)) {
            if ((new_sock = accept(server_sock,
                                     (struct sockaddr *)&server_addr, (socklen_t *)&len)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_sock,
                    inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

            if(send(new_sock, message, strlen(message), 0) != strlen(message)) {
                perror("send");
            }
           if(add_socket(client_socks, new_sock) < 0)
               printf("socket fd array full!");
        }

        //else its some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socks[i];
            if (FD_ISSET(sd , &fdset)) {
                //Check if it was for closing , and also read the
                //incoming message
                if ((read(sd, line, MAX)) == 0) {
                    //Somebody disconnected
                    disconnect_sock(client_socks, server_addr, sd, len, i);
                }
                else {
                    // Client message
                    line[MAX] = '\0';
                    if(strcmp(line, "exit") == 0) {
                        disconnect_sock(client_socks, server_addr, sd, len, i);
                        continue; // avoid sending 'exit' to other clients
                    }
                    // notify all other connected clients about message
                    notify_clients(client_socks, sd, line);
                }
            }
        }
    }
}

