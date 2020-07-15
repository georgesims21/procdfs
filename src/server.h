#ifndef PROCSYS_SERVER_H
#define PROCSYS_SERVER_H

#define MAX_CLIENTS 10

int init_server(int queue_len, struct sockaddr_in *server_add);
int add_clients(int socket_set[], int sd, int maxsd, fd_set *newfdset);
int add_socket(int socket_set[], int new_sock);
void disconnect_sock(int socket_set[], struct sockaddr_in *server_add, int sd, int len, int arrpos);
void notify_clients(int socket_set[], int sd, char *line);
int listenfds(int socket_set[], int server_sock, fd_set *fds, int sd);
void accept_connection(int socket_set[], int server_sock, int new_sock, int len, char *message,
                       struct sockaddr_in *server_add);
char handle_client(int socket_set[], int sd, int len, int i, char *line, struct sockaddr_in *server_add);
void server_loop(int server_sock, int len, char *message);
void run_server(void);

#endif //PROCSYS_SERVER_H
