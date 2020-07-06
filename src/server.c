#include "client-server.h"

struct sockaddr_in server_addr, client_addr;
int mysock, csock;
int r, len, n;

int main(int argc, char *argv[]) {

    printf("Creating TCP stream socket\n");
    if((mysock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    printf("filling sockaddr struct\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Binding socket to server address\n");
    if((r = bind(mysock, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        perror("bind");
        exit(1);
    }

    printf("server is listening\n");
    listen(mysock, 5); // queue length is 5

    char line[MAX];

    while(1) { // for accepting connections
        printf("Server: accepting new connections...\n");
        len = sizeof(client_addr);
        if((csock = accept(mysock, (struct sockaddr *)&client_addr, &len)) < 0 ) {
            perror("accept\n");
            exit(1);
        }
        printf("Connection to client successful\n");

        while(1) { // for processing data from socket
           if((n = recv(csock, line, MAX, 0)) == 0) {
               printf("Connection to client lost\n");
               close(csock);
               break;
           }
           printf("Client message: %s\n", line);
           n = write(csock, line, MAX);
           printf("Server sent message to client, ready for the next request\n");
        }
    }
}

