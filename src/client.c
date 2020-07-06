#include "client-server.h"

struct sockaddr_in server_addr;
int sock, r;

int main(int argc, char *argv[]) {
    printf("Creating TCP stream socket\n");
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    printf("filling sockaddr struct\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Binding socket to server address\n");
    if((r = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        perror("connect");
        exit(1);
    }
    int n;
    char line[MAX], ans[MAX];

    printf("Processing loop\n");
    while(1) {
        bzero(line, MAX);
        fgets(line, MAX, stdin);
        line[strlen(line) - 1] = 0;
        if(line[0] == 0) {
            printf("Nothing entered exiting..\n");
            exit(0);
        }
        n = write(sock, line, MAX);
        printf("Sent message: %s\n", line);
        n = read(sock, ans, MAX);
        printf("Received message: %s\n", line);
    }
}
