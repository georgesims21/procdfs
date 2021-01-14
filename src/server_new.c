#include <arpa/inet.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>

#include "server_new.h"
#include "ds_new.h"

void malloc_error(void) {
    perror("malloc");
    exit(EXIT_FAILURE);
}
void realloc_error(void) {
    perror("realloc");
    exit(EXIT_FAILURE);
}
void calloc_error(void) {
    perror("calloc");
    exit(EXIT_FAILURE);
}

void final_path(const char *path, char *buf) {

    size_t len = strlen("/proc") + strlen(path) + 1;
    char *tmp = malloc(sizeof(char) * len);
    snprintf(tmp, len, "%s%s", "/proc", path);
    memcpy(buf, tmp, len);
    free(tmp);
}

static int fetch_IP(Address *addr, const char *interface) {

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[1025];
    // fetch linked list containing all interfaces on machine
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    // loop through the ll
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            // collect all info for interface
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, 1025, NULL, 0, 1);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            // if this interface matches the specified, save this IP address
            if(strcmp(ifa->ifa_name, interface) == 0) {
                addr->addr.sin_addr.s_addr = inet_addr(host);
                return 0;
            }
        }
    }
    return 1;
}

int init_server(Address *address, int queue_length, int port_number, const char *interface, const char *ip) {

    if((address->sock_in = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }
    if(setsockopt(address->sock_in, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address->addr.sin_family = AF_INET;
    address->addr.sin_port = htons(port_number); // htons converts int to network byte order
    //if(fetch_IP(address, interface) != 0) {
     //   printf("Specified interface: %s does not exist, check spelling and try again\n", interface);
      //  exit(EXIT_FAILURE);
   // }
    address->addr.sin_addr.s_addr = inet_addr(ip);
    address->addr_len = sizeof(address->addr);
    if(bind(address->sock_in, (struct sockaddr*)&address->addr, address->addr_len) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    listen(address->sock_in, queue_length);
    return 0;
}

static int contained_within(Address *addr, Address lookup, int arrlen) {

    Address *ptr = addr; // Address array
    int i = 0;
    while(i < arrlen) {
        if(ptr->addr.sin_addr.s_addr == lookup.addr.sin_addr.s_addr) {
            return i;
        }
        i++; ptr++;
    }
    return -1;
}

static int next_space(Address *addr, int arrlen) {

    // assumes user has locked addr
    int i = 0;
    Address *ptr = addr; // Address array
    while(i < arrlen) {
        if(ptr->sock_in == 0 && ptr->sock_out == 0) {
            return i;
        }
        i++; ptr++;
    }
    return -1;
}

static int pconnect(Address *addrarr, int arrlen, Address *newaddr) {

    int free = 0;
    newaddr->addr.sin_port = htons(1234); // using generic port here, find solution
    int location = 0;
    if((newaddr->sock_out = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }
    if(connect(newaddr->sock_out, (struct sockaddr*)&newaddr->addr, newaddr->addr_len) < 0) {
        close(newaddr->sock_out);
        return -1;
    }
    if((location = contained_within(addrarr, *newaddr, arrlen)) != -1) {
        // if already inside it must be a response from a connected client
        if(addrarr[location].sock_out == 0)
            addrarr[location].sock_out = newaddr->sock_out;
        return -1;
    }
    free = next_space(addrarr, arrlen);
    if(free < 0) {
        printf("[thread: %ld] Maximum number of machines reached (%d), not adding!\n",
               syscall(__NR_gettid), arrlen);
        exit(EXIT_FAILURE);
    }
    addrarr[free] = *newaddr;
    return 0;
}

void *connect_to_file_IPs(void *arg) {

    struct connect_to_file_IPs_args *args = (struct connect_to_file_IPs_args *) arg;
    const char *filename = args->filename;
    char line[31]; // 32 bits for ipv4 address
    Address addresses[args->arrlen];
    int remaining_IPs = args->arrlen;
    in_addr_t conn; // for a sanity check
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        perror("fopen fileip, check if given ip file exists");
        printf("fopen fileip error\n");
        exit(EXIT_FAILURE);
    }
    while(fgets(line, sizeof(line), file)) {
        // skip empty line
        if(strncmp(line, "\n", 1) == 0)
            continue;
        conn = inet_addr(line);
        char tmp[256] = {0};
        // if host IP skip
        if(conn == args->host_addr.addr.sin_addr.s_addr) {
            continue;
        }
        Address new_addr = {0};
        new_addr.addr.sin_addr.s_addr = conn;
        new_addr.addr.sin_family = AF_INET;
        new_addr.addr.sin_port = htons(1234);
        new_addr.addr_len = sizeof(new_addr.addr);
        addresses[args->arrlen - remaining_IPs] = new_addr;
        if((remaining_IPs--) < 0) {
            break;
        }
    }
    remaining_IPs = args->arrlen;
    // attempt to connect to all Addresses in array, skip ones existing in conncli
    while(remaining_IPs > 0) {
        for(int i = 0; i < args->arrlen; i++) {
            // already done
            if(addresses[i].addr.sin_addr.s_addr == 0)
                continue;
            int loc;
            pthread_mutex_lock(args->conn_clients_lock);
            // already inside array
            if((loc = contained_within(args->conn_clients, addresses[i], args->arrlen)) >= 0) {
                // sock_out already done
                if(args->conn_clients[loc].sock_out > 0) {
                    addresses[i].addr.sin_addr.s_addr = 0;
                    pthread_mutex_unlock(args->conn_clients_lock);
                    remaining_IPs--; // not sure about this
                    continue;
                }
            }
            // find empty space in conn_cli array and assign this IPs info to it
            if(pconnect(args->conn_clients, args->arrlen, &addresses[i]) < 0 ) {
                // not added due to already existing, array full or couldn't connect
            } else {
                // pconnect handles adding sock_out if already exists
                addresses[i].addr.sin_addr.s_addr = 0;
                remaining_IPs--;
            }
            pthread_mutex_unlock(args->conn_clients_lock);
        }
//        sleep(1); // allows setup of other machines without using CPU as much
    }
    // check if conn is already located in conn_cli, if yes continue
    fclose(file);
    pthread_exit(0);
}

static void paccept(Address host_addr, Address *client_addr) {

    client_addr->addr_len = sizeof(client_addr->addr);
    if(client_addr->sock_in != 0) {
        printf("[thread: %ld ] sock_in is NOT 0 when accepting the connection!\n", syscall(__NR_gettid));
    }
    if ((client_addr->sock_in = accept(host_addr.sock_in, (struct sockaddr *)&client_addr->addr,
                                       (socklen_t *)&client_addr->addr_len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    getsockname(client_addr->sock_in, (struct sockaddr *)&client_addr->addr, (socklen_t *)client_addr->addr_len);
}

static int search_IPs(in_addr_t conn, const char *filename) {

    char line[32]; // 32 bits for ipv4 address
    in_addr_t newconn;
    FILE* file = fopen(filename, "r");
    if(file < 0) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    while(fgets(line, sizeof(line), file)) {
        // empty line
        if(strncmp(line, "\n", 1) == 0)
            continue;
        newconn = inet_addr(line);
        if(newconn == conn)
            return 0;
    }
    fclose(file);
    return -1;
}

static void *new_connection(void *arg) {

    struct new_connection_args *args = (struct new_connection_args *) arg;
    int loc = 0;
    Address new_client;
    memset(&new_client, 0, sizeof(Address));
    pthread_mutex_lock(args->conn_clients_lock);
    paccept(args->host_addr, &new_client);
    new_client.addr.sin_port = htons(1234);
    if((loc = contained_within(args->conn_clients, new_client, args->arrlen)) >= 0) {
        // already in the array
        args->conn_clients[loc].sock_in = new_client.sock_in;
    } else {
        // not in array, must be added
        if(search_IPs(new_client.addr.sin_addr.s_addr, args->filename) == 0) {
            int free = next_space(args->conn_clients, args->arrlen);
            if(free < 0) {
                printf("[thread: %ld ] Maximum number of machines reached (%d), exiting program..\n",
                       syscall(__NR_gettid), args->arrlen);
                exit(EXIT_FAILURE);
            }
            args->conn_clients[free] = new_client;
        }
    }
    pthread_mutex_unlock(args->conn_clients_lock);
    pthread_exit(0);
}

void *accept_connection(void *arg) {

    struct accept_connection_args *args = (struct accept_connection_args *)arg;
    struct pollfd pfds;
    int remaining_conns = args->arrlen, pollcnt = 0;
    pfds.fd = args->host_addr.sock_in;
    pfds.events = POLLIN;
    while(remaining_conns > 0) {
        if((pollcnt = poll(&pfds, 1, -1)) < 0) { // have 3 sec timeout if doesn't work
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if(pfds.revents & POLLIN) {
            // new connection
            struct new_connection_args nca = {args->conn_clients, args->conn_clients_lock,
                                              args->host_addr, args->arrlen, args->filename};
            pthread_t nc_thread;
            pthread_create(&nc_thread, NULL, new_connection, &nca);
            pthread_join(nc_thread, NULL);
            remaining_conns--; // assumes that if wasn't added correctly then exited
        } else {
            printf("Error on pfds poll\n");
        }
    }
}

static int contained_within_ret(Address *addr, Address *lookup, int arrlen) {

    Address *ptr = addr; // Address array
    int i = 0;
    while(i < arrlen) {
        if(ptr->addr.sin_addr.s_addr == lookup->addr.sin_addr.s_addr) {
            *lookup = *ptr;
            return 0;
        }
        i++; ptr++;
    }
    return -1;
}

int parse_flag(char *buf) {

    // ASCII magic: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
    int flag = buf[0] - '0';
    memmove(buf, buf + 2, strlen(buf));
    return flag;
}

int procsizefd(int fd) {

    char buf[4096];
    int count = 0;
    if(fd < 1 ) {
        // file doesn't exist
        printf("procsizefd fd < 1\n");
        exit(EXIT_FAILURE);
    }
    while(read(fd, buf, 1) > 0) {
        count++;
    }
    return count;
}

char *create_message(Address host_addr, Request *req, int headerlen, int flag) {

    (void)headerlen;
    char hostip[32];
    snprintf(hostip, 32, "%s", inet_ntoa(host_addr.addr.sin_addr));
    char hostpo[16];
    snprintf(hostpo, 16, "%d", htons(host_addr.addr.sin_port));
    if(req->sender.addr.sin_addr.s_addr == 0 || (0 > flag && flag > 3))
        return NULL;
    char header[HEADER] = {0};
    char size[sizeof(size_t)] = {0};
    // create header
    snprintf(header, HEADER, "%d-%s-%s-%s-%hu-%llu-%s-",
             flag,
             hostip,
             hostpo,
             inet_ntoa(req->sender.addr.sin_addr),
             htons(req->sender.addr.sin_port),
             req->atomic_counter,
             req->path
    );
    size_t total_size = strlen(header); // doing another snprintf so don't add term char
    total_size += (flag == FCNT) ? req->buflen : 0;
    // change size into char and get len
    sprintf(size, "%zu", total_size);
    // add total
    size_t stlen = total_size + strlen(size) + 2; // for term char and delim
    char *message = malloc(sizeof(char) * stlen);
    if(!message){malloc_error();};
    if(flag == FCNT) {
        snprintf(message, stlen, "%zu-%s%s",
                 total_size,
                 header,
                 req->buf);
    } else {
        snprintf(message, stlen, "%zu-%s",
                 total_size,
                 header);
    }
    return message;
}

int fetch_upto_delim(char **bufptr, char *buf, int *char_count) {

    while(*(*bufptr) != '-') {
        strncat(buf, *bufptr, 1);
        *(*bufptr)++;
        (*char_count)--;
    }
    *(*bufptr)++;
    return 0;
}

int fetch_size(char *buf, int *counter) {

    char size[sizeof(size_t)] = {0};
    while(buf[*counter] != '-') {
        strncat(size, &buf[*counter], 1);
        (*counter)++;
    }
    (*counter)++; // account for the '-' we discard
    // here len should have string containing total length to read
    int total = atoi(size); // quick and dirty, change to strtol later
    return total;
}

int extract_header(char **bufptr, int *char_count, Request *req, Address host_addr) {

    char senderIP[32] = {0};
    char senderPort[16] = {0};
    char hostIP[32] = {0};
    char hostPort[16] = {0};
    char aa_counter[8] = {0};
    char path[MAXPATH] = {0};
    fetch_upto_delim(bufptr, senderIP, char_count);
    fetch_upto_delim(bufptr, senderPort, char_count);
    fetch_upto_delim(bufptr, hostIP, char_count);
    fetch_upto_delim(bufptr, hostPort, char_count);
    // check whether given hostIP matches actual
    if(inet_addr(hostIP) != host_addr.addr.sin_addr.s_addr ||
       htons(atoi(hostPort)) != host_addr.addr.sin_port) {
        printf("Doesn't match the hostIP address and port, exiting...\n");
        exit(EXIT_FAILURE);
    }
    fetch_upto_delim(bufptr, aa_counter, char_count);
    fetch_upto_delim(bufptr, path, char_count);
    req->sender.addr.sin_addr.s_addr = inet_addr(senderIP);
    req->sender.addr.sin_port = htons(atoi(senderPort));
    req->atomic_counter = strtoll(aa_counter, NULL, 10);
    strncpy(req->path, path, strlen(path));
    return 0;
}

int extract_flag(char **bufptr, int *char_count) {
    // flag
    int flag = -1;
    while(*(*bufptr) != '-') {
        flag = *(*bufptr) - '0';
        *(*bufptr)++;
        (*char_count)--;
    }
    *(*bufptr)++;
    return flag;
}

int extract_buffer(char **bufptr, int *char_count, Request *req, Address host_addr, Address *conn_clients,
                     pthread_mutex_t *conn_clients_lock, int *flag, int arrlen) {

    *flag = extract_flag(bufptr, char_count);
    extract_header(bufptr, char_count, req, host_addr);
    pthread_mutex_lock(conn_clients_lock);
    // use this to find variable in conn_cli, make copy once found
    if((contained_within_ret(conn_clients, &req->sender, arrlen)) == -1) {
        printf("Couldn't find sender address within conn_cli array, exiting..\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(conn_clients_lock);
    return 0;
}

Request *req_create(Address sender, long long counter, const char *path) {

    Request *req = (Request *)malloc(sizeof(Request));
    if(!req){malloc_error();};
    memset(req, 0, sizeof(Request));
    // fill in request struct with info
    req->sender = sender;
    req->atomic_counter = counter;
    snprintf(req->path, MAXPATH, "%s", path);
    return req;
}

int add_inprog(Inprog *inprog, Request *req) {

    // Add to inprog
    req_tracker_ll_add(&inprog->req_ll_head, req);
    inprog->messages_sent++;
    inprog->complete = false;
    return 0;
}

int create_send_msg(Request *req, int flag) {

    int err = 0;
    char *message = create_message(host_addr, req, HEADER, flag);
    if((err = send(req->sender.sock_out, message, strlen(message), 0)) <= 0) {
        if(err < 0) {
            perror("send");
            printf("send error");
            exit(1);
        }
        printf("[thread: %ld ] write to host_client failed\n", syscall(__NR_gettid));
    } else {
        printf("[thread: %ld ] sent %d bytes to %s @ sock_out: %d\n", syscall(__NR_gettid),
               err, inet_ntoa(req->sender.addr.sin_addr), req->sender.sock_out);
    }
    free(message);
    return 0;
}

Inprog *inprog_create(const char *path) {

    Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
    if(!inprog){malloc_error();};
    memset(inprog, 0, sizeof(Inprog));
    inprog->complete = false;
    pthread_mutex_lock(&a_counter_lock);
    a_counter++; // only increment this on new request NOT each machine (reserve 0)
    inprog->atomic_counter = a_counter;
    pthread_mutex_unlock(&a_counter_lock);
    inprog->complete_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(inprog->complete_lock, NULL);
    inprog->complete_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(inprog->complete_cond , NULL);
    inprog->inprog_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(inprog->inprog_lock, NULL);
    for(int i = 0; i < nrmachines; i++) {
        // create request
        Request *req = req_create(connected_clients[i], inprog->atomic_counter, path);
        pthread_mutex_lock(inprog->inprog_lock);
        add_inprog(inprog, req); // adding single request to internal inprog linked list
        pthread_mutex_unlock(inprog->inprog_lock);
        req_tracker_ll_print(&inprog->req_ll_head);
        create_send_msg(req, FREQ);
    } // END of write for loop
    return inprog;
}

Inprog *file_request(const char *path) {

    // create Inprog and lock for it - Inprog now contains ll of all requests sent to other machines
    pthread_mutex_lock(&inprog_tracker_lock);
    Inprog *inprog = inprog_create(path);
    // add node to linked list with this Inprog
    inprog_tracker_ll_add(&inprog_tracker_head, inprog);
    pthread_mutex_unlock(&inprog_tracker_lock);
    pthread_mutex_lock(inprog->complete_lock);
    while(!inprog->complete) {
        pthread_cond_wait(inprog->complete_cond, inprog->complete_lock);
    }
    pthread_mutex_unlock(inprog->complete_lock);
    return inprog;
}

void *server_loop(void *arg) {

    struct server_loop_args *args = (struct server_loop_args *)arg;
    int fdcount = 0, pollcnt = -99;
    struct pollfd *pfds = malloc(sizeof(*pfds) * args->arrlen);
    if(!pfds){malloc_error();};
    // add connected clients to the pfds set
    for(int i = 0; i < args->arrlen; i++) {
        pfds[i].fd = args->conn_clients[i].sock_in;
        pfds[i].events = POLLIN;
        fdcount++;
    }
    for(;;) {
        if((pollcnt = poll(pfds, fdcount, -1)) < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        for(unsigned int i = 0; i < fdcount; i++) {
            if(pfds[i].revents & POLLIN) {
                // reading from already connected client
                int counter = 0, flag = 0;
                Request *req = (Request *)malloc(sizeof(Request));
                if(!req){malloc_error();};
                memset(req, 0, sizeof(Request));
                char *buf = calloc(1, sizeof(size_t) + 1);
                if(!buf){calloc_error();};
                int read_bytes = recv(pfds[i].fd, buf, sizeof(size_t), 0); // assuming we read 8 bytes and not less
                if(read_bytes <= 0) {
                    printf("[thread: %ld ] disconnection, exiting..\n", syscall(__NR_gettid));
                    exit(EXIT_SUCCESS);
                }
                // break down first 8 bytes (size_t), collecting size and whatever is left -----
                int total = fetch_size(buf, &counter);
                int char_count = total;
                buf = realloc(buf, sizeof(char) * total + 1);
                if(!buf){realloc_error();};
                memset(&buf[sizeof(size_t) + 1], 0, (sizeof(char) * total + 1) -(sizeof(size_t) + 1));
                // save after '-' into tmp
                char *contentbuf = calloc(total + 1, sizeof(char));
                if(!contentbuf){calloc_error();};
                char *e_ptr = contentbuf;
                strncpy(contentbuf, &buf[counter], read_bytes - counter + 1);
                int rem_bytes = total - read_bytes + counter; // already read 8 bytes, but size not included in total
                // now read remaining string using sizes we extracted            -----
                while(rem_bytes > 0) {
                    // could have new var from recv and use that in an strncat
                    rem_bytes -= recv(pfds[i].fd, buf, rem_bytes, 0);
                    strcat(contentbuf, buf);
                }
                // get everything from rest of buffer and save into request
                extract_buffer(&e_ptr, &char_count, req, args->host_addr, args->conn_clients,
                               args->conn_clients_lock, &flag, args->arrlen);
                switch(flag) {
                    case FREQ: { // 1: other machine requesting file content
                        int fd = -1, res = 0, offset = 0, size = 0, err = 0;
                        fd = openat(-100, req->path, O_RDONLY);
                        if (fd == -1) {
                            perror("openat");
                            printf("%s\n", req->path);
                            exit (EXIT_FAILURE);
                        }
                        size = procsizefd(fd); // individually count chars in proc file - bottleneck for large fs
                        char *procbuf = malloc(sizeof(char) * size);
                        if(!procbuf){malloc_error();};
                        res = pread(fd, procbuf, size, offset);
                        if (res == -1) {
                            perror("pread");
                            exit(EXIT_FAILURE);
                        }
                        req->buflen = size;
                        req = realloc(req, sizeof(Request) + req->buflen); // flexible array member use
                        if(!req){realloc_error();};
                        memset(req->buf, 0, sizeof(req->buflen));
                        snprintf(req->buf, req->buflen, "%s", procbuf);
                        free(procbuf); // -- culprit to seg fault
                        // send the req back to the sender (should use new create_and_send_msg method)
                        char *message = create_message(args->host_addr, req, HEADER, FCNT);
                        if((err = send(req->sender.sock_out, message, strlen(message) + 1, 0)) <= 0) {
                            if(err < 0) {
                                perror("send");
                            }
                            printf("[thread: %ld] write to host_client failed\n", syscall(__NR_gettid));
                        }
                        free(message);
                        close(fd);
                        break;
                    }
                    case FCNT: // 2: this machine receiving a response with content
                        req->buflen = char_count + 1; // char count includes 0 index so must add 1
                        req = realloc(req, sizeof(Request) + req->buflen);
                        if(!req){realloc_error();};
                        memset(req->buf, 0, req->buflen);
                        // buffer, no delim so must just go on rest of size - done in switch
                        while(char_count >= 0) {
                            strncat(req->buf, e_ptr, 1);
                            e_ptr++;
                            char_count--;
                        }
                        pthread_mutex_lock(&inprog_tracker_lock);
                        // find this request in the upper linked list and return node
                        struct inprog_tracker_node *node = inprog_tracker_ll_fetch_req(&inprog_tracker_head, *req);
                        // add file buf received from other machine to the request (also checks if Inprog == complete)
                        inprog_add_buf(req, node->inprog, node->inprog->inprog_lock);
                        inprog_tracker_ll_print(&inprog_tracker_head);
                        pthread_mutex_unlock(&inprog_tracker_lock);
                        break;
                    default:
                        break;
                }
                free(req);
                free(buf);
                free(contentbuf);
            }
        } // END of poll loop
    } // END of inf for loop
} // END of server loop