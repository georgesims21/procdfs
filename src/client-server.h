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
#define MAX 256
#define SERVER_HOST "localhost"
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
struct sockaddr_in server_addr;

#define CONN_MSG 1
#define REG_MSG 2

#endif //PROCSYS_CLIENT_SERVER_H
