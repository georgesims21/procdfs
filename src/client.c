#include "client-server.h"
#include "client.h"

int init_client(struct sockaddr_in *server_add) {
    int sock, r;

    printf("Creating TCP stream socket\n");
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    printf("filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Binding socket to server address\n");
    if((r = connect(sock, (struct sockaddr*)server_add, sizeof(*server_add))) < 0) {
        perror("connect");
        exit(1);
    }

    return sock;
}

void write_loop(char *line, int sock) {
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
}

void read_loop(char *ans, int sock) {
    int n;

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

void read_write_loop(int sock) {
    char line[MAX] ={0}, ans[MAX] = {0};
    pid_t pid;

    if((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    } else if(pid == 0) {
        // child
        printf("child initiated\n");
        write_loop(line, sock);

    } else {
        // parent
        printf("Processing loop\n");
        read_loop(ans, sock);
    }
}

void run_client(void) {
    int sock;

    sock = init_client(&server_addr);
    read_write_loop(sock);
}

int main(int argc, char *argv[]) {
    run_client();
    return 0;
}
