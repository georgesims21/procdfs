#include "client-server.h"

int sock, r;

int main(int argc, char *argv[]) {
    printf("Creating TCP stream socket\n");
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    printf("Binding socket to server address\n");
    if((r = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        perror("connect");
        exit(1);
    }
    int n;
    char line[MAX] ={0}, ans[MAX] = {0};
    pid_t pid;

    if((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    } else if(pid == 0) {
        // child
        printf("child initiated\n");
        while(1) {
            bzero(line, MAX);
            fgets(line, MAX, stdin);
            line[strlen(line) - 1] = 0;
            if(line[0] == 0) {
                printf("Nothing entered exiting..\n");
                exit(0);
            }
            if((write(sock, line, MAX)) < 0) {
                perror("write");
                exit(1);
            }
            printf("Sent message: %s\n", line);
            if(strcmp(line, "exit") == 0)
                exit(0);
        }
    } else {
        // parent
        printf("Processing loop\n");
        while(1) {
            if((n = read(sock, ans, MAX)) == 0) {
                perror("read");
                exit(1);
            } else if(n == 0) {
                printf("Server disconnected.. exiting");
                exit(1);
            }
            printf("Received message: %s\n", ans);
            memset(ans, 0, sizeof(ans));
        }
    }
}
