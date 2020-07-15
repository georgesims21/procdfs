#ifndef PROCSYS_CLIENT_H
#define PROCSYS_CLIENT_H

int init_client(struct sockaddr_in *server_add);
void write_loop(char *line, int sock);
void read_loop(char *ans, int sock);
void read_write_loop(int sock);
void run_client(void);

#endif //PROCSYS_CLIENT_H
