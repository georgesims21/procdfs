#include "client-server.h"

struct sockaddr_in server_addr, client_addr;
int mysock, csock;
int r, len, n;

/*
 * TODO
 *  - [x] Basic client-server model
 *  - [ ] Hold more than 1 connection (with comms.)
 *  - [ ] Ping messages from server to all clients (notification)
 */

int main(int argc, char *argv[]) {

    fd_set fdset;
    int client_socks[10], max_clients = 10, opt = 1, server_sock = 0, new_sock = 0, sd, maxsd, activity, errno;
    unsigned int i = 0;
    char line[MAX], message[] = "Connected to server...";

    printf("Creating TCP stream socket\n");
    if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    if( setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    printf("filling sockaddr struct\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Binding socket to server address\n");
    if((r = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        perror("bind");
        exit(1);
    }

    printf("server is listening\n");
    listen(server_sock, 5); // queue length is 5

    for (i = 0; i < max_clients; i++) {
        client_socks[i] = 0;
    }

    len = sizeof(client_addr);

    while(1) { // for accepting connections

        printf("Initializing sd set");
        // Socket set logic
        FD_ZERO(&fdset);
        FD_SET(server_sock, &fdset);
        maxsd = server_sock;

        for(i = 0; i < max_clients; i++) {
            if((sd = client_socks[i]) > 0)
                FD_SET(sd, &fdset); // if valid add to set

            if(sd > max_clients)
                maxsd = sd; // Need higheset sd for select
        }

//        activity = select( max_clients + 1 , &fdset , NULL , NULL , NULL);
//        if ((activity < 0) && (errno != EINTR)) {
//            printf("select error");
//        }
        printf("Server: listening to fdset...\n");
        if((select( max_clients + 1 , &fdset , NULL , NULL , NULL)) < 0) // indefinitely
            printf("Select error\n");

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(server_sock, &fdset)) {
            if ((new_sock = accept(server_sock,
                                     (struct sockaddr *)&client_addr, &len))<0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_sock , inet_ntoa(client_addr.sin_addr) , ntohs
                    (client_addr.sin_port));

            //send new connection greeting message
            if(send(new_sock, message, strlen(message), 0) != strlen(message)) {
                perror("send");
            }

            printf("Welcome message sent successfully\n");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                //if position is empty
                if(client_socks[i] == 0) {
                    client_socks[i] = new_sock;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        }

        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++) {
            sd = client_socks[i];

            if (FD_ISSET(sd , &fdset)) {
                //Check if it was for closing , and also read the
                //incoming message
                if ((read(sd, line, MAX)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr*)&client_addr , &len);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socks[i] = 0;
                }

                    //Echo back the message that came in
                else {
                    //set the string terminating NULL byte on the end
                    //of the data read
                    line[MAX] = '\0';
                    send(sd , line , strlen(line) , 0 );
                }
            }
        }

//        if((csock = accept(mysock, (struct sockaddr *)&client_addr, &len)) < 0 ) {
//            perror("accept\n");
//            exit(1);
//        }
//        printf("Connection to client successful\n");
//
//        while(1) { // for processing data from socket
//           if((n = recv(csock, line, MAX, 0)) == 0) {
//               printf("Connection to client lost\n");
//               close(csock);
//               break;
//           }
//           printf("Client message: %s\n", line);
//           n = write(csock, line, MAX);
//           printf("Server sent message to client, ready for the next request\n");
        }
    }

