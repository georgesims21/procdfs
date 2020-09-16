#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include "server_new.h"
#include "ds_new.h"

char tb[26];

void get_time(char *buf) {
//    get_time(tb);
//    printf("[thread: %ld {%s}] Listening on :%s at port: %d\n", syscall(__NR_gettid), tb,
//           inet_ntoa(address->addr.sin_addr), htons(address->addr.sin_port));
    memset(tb, 0, sizeof(tb));
    int millisec;
    struct tm* tm_info;
    struct timeval tv;
    char buffer[26];
    gettimeofday(&tv, NULL);

    millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        tv.tv_sec++;
    }
    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 26, "%M:%S", tm_info);
    sprintf(buf, "%s.%03d", buffer, millisec);
}

int init_server(Address *address, int queue_length, int port_number, const char *interface) {

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
    if(fetch_IP(address, interface) != 0) {
        printf("Specified interface: %s does not exist, check spelling and try again\n", interface);
        exit(EXIT_FAILURE);
    }
    address->addr_len = sizeof(address->addr);
    if(bind(address->sock_in, (struct sockaddr*)&address->addr, address->addr_len) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    listen(address->sock_in, queue_length);
    return 0;
}

void *connect_to_file_IPs(void *arg) {

    struct connect_to_file_IPs_args *args = (struct connect_to_file_IPs_args *) arg;
    const char *filename = args->filename;
    char line[31]; // 32 bits for ipv4 address
    Address addresses[args->arrlen];
    int remaining_IPs = args->arrlen;
    in_addr_t conn;

    FILE* file = fopen(filename, "r");
    if((file < 0)) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // save all IPs from file to an Address array
    int count = 0;
    while(fgets(line, sizeof(line), file)) {
        if(strncmp(line, "\n", 1) == 0)
            continue;
        conn = inet_addr(line);
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
        sleep(1);
    }
    // check if conn is already located in conn_cli, if yes continue
    fclose(file);
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
                get_time(tb);
                printf("[thread: %ld {%s}] Maximum number of machines reached (%d), exiting program..\n",
                       syscall(__NR_gettid), tb, args->arrlen);
                exit(EXIT_FAILURE);
            }
            args->conn_clients[free] = new_client;
        }
    }
    pthread_mutex_unlock(args->conn_clients_lock);
    pthread_exit(0);
}

static void paccept(Address host_addr, Address *client_addr) {

    client_addr->addr_len = sizeof(client_addr->addr);

    if(client_addr->sock_in != 0) {
        get_time(tb);
        printf("[thread: %ld {%s}] sock_in is NOT 0 when accepting the connection!!\n", syscall(__NR_gettid), tb);
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

static int fetch_IP(Address *addr, const char *interface) {

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

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
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
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
        get_time(tb);
        printf("[thread: %ld {%s}] Maximum number of machines reached (%d), not adding!\n",
               syscall(__NR_gettid), tb, arrlen);
        exit(EXIT_FAILURE);
    }

    addrarr[free] = *newaddr;
    return 0;
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
    if(fd < 0)
        // file doesn't exist
        return -1;
    while(read(fd, buf, 1) > 0) {
        count++;
    }
    return count;
}

char *create_message(Address host_addr, Request *req, int headerlen, int flag) {

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
    char *message = malloc(stlen);
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
    char a_counter[8] = {0};
    char path[MAXPATH] = {0};

    fetch_upto_delim(bufptr, senderIP, char_count);
    fetch_upto_delim(bufptr, senderPort, char_count);
    fetch_upto_delim(bufptr, hostIP, char_count);
    fetch_upto_delim(bufptr, hostPort, char_count);
    // check whether given hostIP matches actual
    if(!(inet_addr(hostIP) == host_addr.addr.sin_addr.s_addr) ||
       !(htons(atoi(hostPort)) == host_addr.addr.sin_port)) {
        printf("Doesn't match the hostIP address and port, exiting...\n");
        exit(EXIT_FAILURE);
    }
    fetch_upto_delim(bufptr, a_counter, char_count);
    fetch_upto_delim(bufptr, path, char_count);

    req->sender.addr.sin_addr.s_addr = inet_addr(senderIP);
    req->sender.addr.sin_port = htons(atoi(senderPort));
    req->atomic_counter = strtoll(a_counter, NULL, 10);
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

void *server_loop(void *arg) {

    struct server_loop_args *args = (struct server_loop_args *)arg;
    int fdcount = 0, pollcnt = -99;
    struct pollfd *pfds = malloc(sizeof(*pfds) * args->arrlen);
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
                /*
                 * TODO
                     * switch
                         * 1: file request
                             * (extra) make into void method
                         * 2: content received
                             * (extra) make into void method
                     * Going to need the inprog_tracker_node ll as global so fuse can access
                 */
                int counter = 0, flag = 0;
                Request *req = (Request *)malloc(sizeof(Request));
                memset(req, 0, sizeof(Request));
                char *buf = calloc(0, sizeof(size_t) + 1);
                int read_bytes = recv(pfds[i].fd, buf, sizeof(size_t), 0); // assuming we read 8 bytes and not less
                printf("Received %d bytes\n", read_bytes);
                if(read_bytes <= 0) {
                    get_time(tb);
                    printf("[thread: %ld {%s}] disconnection, exiting..\n", syscall(__NR_gettid), tb);
                    exit(EXIT_SUCCESS);
                }
                int total = fetch_size(buf, &counter);
                int char_count = total;
                buf = realloc(buf, sizeof(char) * total + 1);
                // save after '-' into tmp
                char *contentbuf = malloc(sizeof(char) * total + 1);
                char *e_ptr = contentbuf;
                strncpy(contentbuf, &buf[counter], read_bytes - counter + 1);
                int rem_bytes = total - read_bytes + counter; // already read 8 bytes, but size not included in total
                while(rem_bytes > 0) {
                    // could have new var from recv and use that in an strncat
                    rem_bytes -= recv(pfds[i].fd, buf, rem_bytes, 0);
                    strcat(contentbuf, buf);
                }
                get_time(tb);
                printf("[thread: %ld {%s}] (%d) bytes received: %s\n", syscall(__NR_gettid), tb,
                       total, contentbuf);
                // now we have a request struct we can switch for what we need
                extract_buffer(&e_ptr, &char_count, req, args->host_addr, args->conn_clients,
                               args->conn_clients_lock, &flag, args->arrlen);
                switch(flag) {
                    case FREQ: { // 1: other machine requesting file content
                        printf("File request received\n");
                        int fd = -1, res = 0, offset = 0, size = 0, err = 0;
                        fd = openat(AT_FDCWD, req->path, O_RDONLY);
                        if (fd == -1) {
                            perror("openat");
                            exit (EXIT_FAILURE);
                        }
                        size = procsizefd(fd);
                        char *procbuf = malloc(sizeof(char) * size);
                        printf("procsize: %d\n", size);

                        res = pread(fd, procbuf, size, offset);
                        if (res == -1) {
                            perror("pread");
                            exit(EXIT_FAILURE);
                        }
                        req->buflen = size;
                        req = realloc(req, sizeof(Request) + req->buflen);
                        memset(req->buf, 0, sizeof(req->buflen));
                        snprintf(req->buf, req->buflen, "%s", procbuf);
                        free(procbuf);
                        // send the req back to the sender
                        char *message = create_message(args->host_addr, req, HEADER, FCNT);
                        printf("FCNT Sending: %s\n", message);
                        if((err = send(req->sender.sock_out, message, strlen(message) + 1, 0)) <= 0) {
                            if(err < 0) {
                                perror("send");
                            }
                            get_time(tb);
                            printf("[thread: %ld {%s}] write to host_client failed\n", syscall(__NR_gettid), tb);
                        } else {
                            get_time(tb);
                            printf("[thread: %ld {%s}] sent %d bytes to %s @ sock_out: %d\n", syscall(__NR_gettid),
                                   tb, err, inet_ntoa(req->sender.addr.sin_addr), req->sender.sock_out);
                        }
                        free(message);
                        break;
                    }
                    case FCNT: // 2: this machine receiving a response with content
                        printf("File content received\n");
                        req->buflen = char_count + 1; // char count includes 0 index so must add 1
                        req = realloc(req, sizeof(Request) + req->buflen);
                        memset(req->buf, 0, req->buflen);
                        // buffer, no delim so must just go on rest of size - done in switch
                        while(char_count >= 0) {
                            strncat(req->buf, e_ptr, 1);
                            e_ptr++;
                            char_count--;
                        }
                        inprog_add_buf(req, args->inprog, args->inprog_lock);
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

//int main(int argc, char *argv[]) {
//
//    if(argc < 4) {
//        printf("Not enough arguments given, 4 expected: total-machines port-number interface-name ipfile\n");
//        exit(EXIT_FAILURE);
//    }
//    long nrm = strtol(argv[1], NULL, 10);
//    long pnr = strtol(argv[2], NULL, 10);
//    const char *infc = argv[3];
//    const char *fn = argv[4];
//    // Check if returned error from strtol OR if the longs are too large to convert
//    if (errno != 0 || ((nrm > INT_MAX) || (pnr > INT_MAX ))) {
//        printf("%s argument too large!\n", (nrm > INT_MAX) ? "first" : "second");
//        exit(EXIT_FAILURE);
//    }
//    int err;
//    int nrmachines = (int)nrm - 1; // to account for this machine (not adding to connected clients)
//    int portnr = (int)pnr;
//
//    printf("Connecting to other machines..\n");
//    Address host_addr;
//    memset(&host_addr, 0, sizeof(host_addr));
//    init_server(&host_addr, nrmachines, portnr, infc);
//
//    // init Address arrays and their corresponding mutex locks
//    Address connected_clients[nrmachines];
//    memset(connected_clients, 0, sizeof(Address) * (nrmachines));
//    pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;;
//
//    Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
//    if(inprog == NULL) {
//        perror("malloc");
//        exit(EXIT_FAILURE);
//    }
//    memset(inprog, 0, sizeof(Inprog));
//    pthread_mutex_t inprog_lock = PTHREAD_MUTEX_INITIALIZER;
//
//    // accept all incoming connections until have sock_in for all machines in list
//    struct accept_connection_args aca = {connected_clients, &connected_clients_lock,
//                                         host_addr, nrmachines, fn};
//    pthread_t aca_thread;
//    pthread_create(&aca_thread, NULL, accept_connection, &aca);
//    // connect to IPs in ipfile
//    struct connect_to_file_IPs_args ctipa = {connected_clients, &connected_clients_lock,
//                                             host_addr, nrmachines, fn};
//    pthread_t ctipa_thread;
//    pthread_create(&ctipa_thread, NULL, connect_to_file_IPs, &ctipa);
//    // force main to wait until connected to all machines
//    pthread_join(ctipa_thread, NULL);
//    pthread_join(aca_thread, NULL);
//
//    printf("You are now connected to machines: \n");
//    for(int j = 0; j < nrmachines; j++) {
//        printf("%s\t@\t%d\n",
//               inet_ntoa(connected_clients[j].addr.sin_addr),
//               htons(connected_clients[j].addr.sin_port));
//    }
//    printf("on this address: \n%s\t@\t%d\n",
//           inet_ntoa(host_addr.addr.sin_addr),
//           htons(host_addr.addr.sin_port));
//
//    // start server loop to listen for connections
//    struct server_loop_args sla = {connected_clients, &connected_clients_lock,
//                                   inprog, &inprog_lock,
//                                   host_addr, nrmachines, fn};
//    pthread_t sla_thread;
//    pthread_create(&sla_thread, NULL, server_loop, &sla);
//
//    long long a_counter = 0;
//    pthread_mutex_t a_counter_lock = PTHREAD_MUTEX_INITIALIZER;
//    char buf[1024];
//    Address ad;
//    for(;;) {
//        printf("~ ");
//        scanf("%s", buf);
//        pthread_mutex_lock(&a_counter_lock);
//        a_counter++; // only increment this on new request NOT each machine
//        for(int i = 0; i < nrmachines; i++) {
//            // create request
//            Request *req = (Request *)malloc(sizeof(Request));
//            memset(req, 0, sizeof(Request));
//            // fill in request struct with info
//            req->sender = connected_clients[i];
//            req->atomic_counter = inprog->atomic_counter = a_counter;
//            pthread_mutex_unlock(&a_counter_lock);
//            snprintf(req->path, MAXPATH, "/proc/net/dev");
//            // Add to inprog
//            pthread_mutex_lock(&inprog_lock);
//            req_tracker_ll_add(&inprog->req_ll_head, req);
//            inprog->messages_sent++;
//            inprog->complete = false;
//            req_tracker_ll_print(&inprog->req_ll_head);
//            pthread_mutex_unlock(&inprog_lock);
//
//            char *message = create_message(host_addr, req, HEADER, FREQ);
//            printf("FREQ Sending: %s\n", message);
//
//            if((err = send(req->sender.sock_out, message, strlen(message), 0)) <= 0) {
//                if(err < 0) {
//                    perror("send");
//                }
//                get_time(tb);
//                printf("[thread: %ld {%s}] write to host_client failed\n", syscall(__NR_gettid), tb);
//            } else {
//                get_time(tb);
//                printf("[thread: %ld {%s}] sent %d bytes to %s @ sock_out: %d\n", syscall(__NR_gettid),
//                       tb, err, inet_ntoa(req->sender.addr.sin_addr), req->sender.sock_out);
//            }
//            free(message);
//        } // END of write for loop
//    } // END of inf for loop
//    return 0;
//}
