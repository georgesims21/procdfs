#include "client-server.h"
#include "client.h"
#include "log.h"

struct sockaddr_in server_addr;

int init_client(struct sockaddr_in *server_add) {
    int sock, r;

    lprintf("{client}Creating TCP stream socket\n");
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    lprintf("{client}filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    lprintf("{client}Binding socket to server address\n");
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
            lprintf("{client}Nothing entered exiting..\n");
            exit(0);
        }
        if((write(sock, line, MAX)) < 0) {
            perror("write");
            exit(1);
        }
        lprintf("{client}Sent message: %s\n", line);
        if(strcmp(line, "exit") == 0)
            exit(0);
    }
}

void write_server(char *line, int sock) {
    if((write(sock, line, MAX)) < 0) {
        perror("write");
        exit(1);
    }
    lprintf("{client}Sent message to server: %s\n", line);

}

int parse_message(char *message) {
    // ASCII magic: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
    int flag = message[0] - '0';
    memmove(message, message + 1, strlen(message));
    return flag;
}

void read_loop(int sock) {
    /*
     * TODO
     *  bug : getting 0 from read call, server disconnects when client connects
     */

    int n;
    char ans[MAX] = {0};

    while(1) {
        if((n = read(sock, ans, MAX)) < 0) {
            perror("read");
            exit(1);
        } else if(n == 0) {
            lprintf("{client}Server disconnected.. exiting\n");
            exit(1);
        }
        switch(parse_message(ans)) {
            case CONN_MSG:
                lprintf("{client}[connection message] %s\n", ans);
                break;
            case REG_MSG:
                // Will need to run methods from here when server requests a file
                lprintf("{client}[regular message] %s\n", ans);
                break;
            default:
                lprintf("{client}[message] %s\n", ans);
                break;
        }
        memset(ans, 0, sizeof(ans));
    }
}

void read_server(int sock) {
    /*
     * TODO
     *  [ ] log function not writing messages
     *      - Server is disconnecting, check return values of read to see if error checking is right
     */

    int n;
    char ans[MAX] = {0};

    if((n = read(sock, ans, MAX)) < 0) {
        perror("read");
        exit(1);
    } else if(n == 0) {
        lprintf("{client}Server disconnected.. exiting\n");
        exit(1);
    }
    switch(parse_message(ans)) {
        case CONN_MSG:
            lprintf("{client}[connection message] %s\n", ans);
            break;
        case REG_MSG:
            lprintf("{client}[regular message] %s\n", ans);
            break;
        default:
            lprintf("{client}[message] %s\n", ans);
            break;
    }
    memset(ans, 0, sizeof(ans));
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

int main(int argc, char *argv[]) {
    run_client();
    return 0;
}
