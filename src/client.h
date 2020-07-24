#ifndef PROCSYS_CLIENT_H
#define PROCSYS_CLIENT_H
#include <setjmp.h>

int init_client(struct sockaddr_in *server_add);
void write_loop(int sock);
void read_loop(int sock);
void read_write_loop(int sock);
int parse_message(char *message);
void lprintf(const char *fmt, ...);
void read_server(int sock);
void write_server(char *line, int sock);
void run_client(void);

extern jmp_buf readjmpbuf;

#endif //PROCSYS_CLIENT_H
