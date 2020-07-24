#include "client-server.h"
#include "client.h"
#include "log.h"
#include "fileops.h"
#include "reader.h"
#include "writer.h"
#include "queue.h"

struct sockaddr_in server_addr;
QUEUE *queue;

/*
 * TODO
 *  * writer
 *  - [x] make prepend flag method
 *  - [ ] send to server method loop
 *       - only sends if queue is non-empty, can make a global flag for now with bufsize
 *          * this loop uses excess processing power when checking the queue - need another solution
 *  * reader
 *  - [x] make parse flag method - returns file path(share in reader api)
 *  - [x] make method to fetch file content into buf
 *  - [ ] longjmp() back to the client FS
 *  * main
 *  - [ ] take final buf and return it to the (read()) method
 */

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

void write_loop(int sock) {
    char line[MAX] = {0};

    BUFELEM *tmpbuf = {0};
    while(1) {
        if(lenq(&queue)) {
            memset(tmpbuf, 0, sizeof(&tmpbuf));
            tmpbuf = dequeue(&queue);
            if((write(sock, tmpbuf->buf, strlen(tmpbuf->buf))) < 0) {
                perror("write");
                exit(1);
            }
        }
    }
}


void read_loop(int sock) {
    /*
     * TODO
     *  [ ] in the switch, handle incoming server [messages]
     *      - find the file requested
     *      - problem is: need to stay in fs operations so we can print BUT also need a constant
     *      loop reading from the server in case other fs' need a file at any time
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
        switch(parse_flag(ans)) {
            case CONN_MSG_SER:
                lprintf("{client}[connection message] %s\n", ans);
                break;
            case REQ_MSG_SER:
                /*
                 * TODO
                 *  [ ] prepend CNT_MSG_CLI to message content buffer before sending
                 */
                lprintf("{client}[file request]for path: \"%s\"\n", ans);
                // from here content of the file is fetched and sent to the server
                int fd = -1, res = 0, offset = 0, size = 0;
                char buf[4096] = {0};
                fd = openat(AT_FDCWD, ans, O_RDONLY);
                if (fd == -1) {
                    perror("openat");
                    exit (EXIT_FAILURE);
                }
                size = procsizefd(fd);

                res = pread(fd, buf, size, offset);
                if (res == -1) {
                    perror("pread");
                    exit(EXIT_FAILURE);
                }
                prepend_flag(CNT_MSG_CLI, buf);
                if((write(sock, buf, strlen(buf))) < 0) {
                    perror("write");
                    exit(1);
                }
                break;
            case FIN_MSG_SER:
                lprintf("{client}[final content received] %s\n", ans);
                // from here content of the buffer is handled by the fs and sent to the user
                break;
            default:
                lprintf("{client}[other] %s\n", ans);
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
        write_loop(sock);
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
