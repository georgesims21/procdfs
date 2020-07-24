#ifndef PROCSYS_CLIENT_SERVER_H
#define PROCSYS_CLIENT_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <asm/errno.h>
#include <libnet.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define MAX 4096
#define READ_MAX (MAX-2)
#define TERM_CHAR_MAX (MAX-1)


#define SERVER_HOST "localhost"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define SIZEOF_INT sizeof(int)

extern struct sockaddr_in server_addr;
extern int server_sock;

// server->client comms flags
#define CONN_MSG_SER    1 // initial connection message
#define REQ_MSG_SER     2 // requesting file content (other client requested)
#define FIN_MSG_SER     3 // contains file content    (this client requested)
// client->server comms flags
#define CNT_MSG_CLI     4 // replying with file content
#define NME_MSG_CLI     5 // replying with file name

#endif //PROCSYS_CLIENT_SERVER_H
