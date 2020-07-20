#include <syslog.h>
#include "client-server.h"
#include "client.h"

struct sockaddr_in server_addr;

int init_client(struct sockaddr_in *server_add) {
    int sock, r;

    lprintf("Creating TCP stream socket\n");
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    lprintf("filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    lprintf("Binding socket to server address\n");
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
            lprintf("Nothing entered exiting..\n");
            exit(0);
        }
        if((write(sock, line, MAX)) < 0) {
            perror("write");
            exit(1);
        }
        lprintf("Sent message: %s\n", line);
        if(strcmp(line, "exit") == 0)
            exit(0);
    }
}

int parse_message(char *message) {
    // ASCII magic: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
    int flag = message[0] - '0';
    memmove(message, message + 1, strlen(message));
    return flag;
}

//void pprintf(char *message) {
//    FILE *fp;
//    chdir("/home/george/github/procsys");
//
//    if((fp = fopen("procsys.log", "w+")) < 0 ) {
//        perror("fopen");
//        exit(EXIT_FAILURE);
//    }
//
//    fprintf(fp, "%s\n", message);
//    fclose(fp);
//}

void lprintf(const char *fmt, ...) {
//    https://stackoverflow.com/questions/7031116/how-to-create-function-like-printf-variable-argument
    va_list arg;
    FILE *fp;
    chdir("/home/george/github/procsys");
    fp = fopen("procsys.log", "a+");
    /* Write the error message */
    va_start(arg, fmt);
    vfprintf(fp, fmt, arg);
    va_end(arg);
    fclose(fp);
}

void read_loop(int sock) {

    int n;
    char ans[MAX] = {0};

    while(1) {
        if((n = read(sock, ans, MAX)) < 0) {
            perror("read");
            exit(1);
        } else if(n == 0) {
            lprintf("Server disconnected.. exiting");
            exit(1);
        }
        switch(parse_message(ans)) {
            case CONN_MSG:
                lprintf("[connection message] %s\n", ans);
                break;
            default:
                lprintf("[message] %s\n", ans);
                break;
        }
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
        write_loop(line, sock);

    } else {
        // parent
        read_loop(sock);
    }
}

void run_client(void) {
    int sock;

    sock = init_client(&server_addr);
    read_write_loop(sock);
}
//
//int main(int argc, char *argv[]) {
//    run_client();
//    return 0;
//}
