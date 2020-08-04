#include "client-server.h"
#include "client.h"
#include "log.h"
#include "fileops.h"
#include "reader.h"
#include "writer.h"
#include "queue.h"
#include "defs.h"

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
 *  - [x] create a pipe to the reader process
 *  - [x] force the fs to block on a read call to the pipe, if data then it's the final message
 *      - Is this method safe? i.e think about if the fs needs to read twice or in multithreaded mode
 *  - [ ] make sure all buffers are memset correctly after each read etc, getting junk on some
 *  * main
 *  - [ ] take final buf and return it to the (read()) method
 */

int init_client(struct sockaddr_in *server_add) {
    int sock, r;

    lprintf("{client %d}Creating TCP stream socket\n", getpid());
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(1);
    }

    lprintf("{client %d}filling sockaddr struct\n", getpid());
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    lprintf("{client %d}Binding socket to server address\n", getpid());
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

    int n;
    char ans[MAX] = {0};

    while(1) {
        if((n = read(sock, ans, MAX)) < 0) {
            perror("read");
            exit(1);
        } else if(n == 0) {
            lprintf("{client %d}Server disconnected.. exiting\n", getpid());
            exit(1);
        }
        switch(parse_flag(ans)) {
            case CONN_MSG_SER:
                lprintf("{client %d}[connection message] %s\n", getpid(), ans);
                break;
            case REQ_MSG_SER:
                lprintf("{client %d}[file request]for path: \"%s\"\n", getpid(), ans);
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
                lprintf("{client %d}[final content received] %s\n", getpid(), ans);
                write(pipecomms[1], ans, sizeof(ans));
                // from here content of the buffer is handled by the fs and sent to the user
                break;
            default:
                lprintf("{client %d}[other] %s\n", getpid(), ans);
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
