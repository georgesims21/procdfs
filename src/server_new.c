#include <arpa/inet.h>
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
#include "server_new.h"
#include "ds_new.h"

char tb[26];

void get_time(char *buf) {
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
    get_time(tb);
    printf("[thread: %ld {%s}] Listening on :%s at port: %d\n", syscall(__NR_gettid), tb,
           inet_ntoa(address->addr.sin_addr), htons(address->addr.sin_port));
    return 0;
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
    newaddr->addr.sin_family = AF_INET;
    newaddr->addr.sin_port = htons(1234); // using generic port here, find solution
    newaddr->addr_len = sizeof(newaddr->addr);
    get_time(tb);
    printf("[thread: %ld {%s}] checking if client is inside the host_client array\n", syscall(__NR_gettid), tb);
    int location = 0;

    free = next_space(addrarr, arrlen);
    if(free < 0) {
        get_time(tb);
        printf("[thread: %ld {%s}] Maximum number of machines reached (%d), not adding!\n",
               syscall(__NR_gettid), tb, arrlen);
        return -3;
    }
    if((location = contained_within(addrarr, *newaddr, arrlen)) == -1) {
        // if not inside already
        if((newaddr->sock_out = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket\n");
            exit(EXIT_FAILURE);
        }
        if(connect(newaddr->sock_out, (struct sockaddr*)&newaddr->addr, newaddr->addr_len) < 0) {
            get_time(tb);
            printf("[thread: %ld {%s}] Cannot connect to %s @ sock %d\n", syscall(__NR_gettid), tb,
                   inet_ntoa(newaddr->addr.sin_addr), newaddr->sock_out);
            close(newaddr->sock_out);
            return -2;
        }
        get_time(tb);
        printf("[thread: %ld {%s}] IP: %s now connected @ %d\n", syscall(__NR_gettid), tb,
               inet_ntoa(newaddr->addr.sin_addr), newaddr->sock_out);
    } else {
        // if already inside
        get_time(tb);
        printf("[thread: %ld {%s}] Address already inside array, adding sock_in: %d\n",
               syscall(__NR_gettid), tb, newaddr->sock_in);
        if(addrarr[location].sock_in == 0)
            addrarr[location].sock_in = newaddr->sock_in;
        return -1;
    }


    addrarr[free] = *newaddr;
//    Address *ptr = &args->conn_clients[free]; // Address array
//    *ptr = newaddr;
    get_time(tb);
    printf("[thread: %ld {%s}] Added %s to conn_clients[%d]: %s and connected to port %d @ socket: %d\n",
           syscall(__NR_gettid), tb, inet_ntoa(newaddr->addr.sin_addr), free,
            inet_ntoa(addrarr[free].addr.sin_addr),
            addrarr[free].addr.sin_port,
            addrarr[free].sock_out);

    get_time(tb);
    printf("[thread: %ld {%s}] printing conn clients: \n", syscall(__NR_gettid), tb);
    for(int j = 0; j < arrlen; j++) {
        printf("{connected} conn_clients[%d]: \nsock_in: %d\nsock_out: %d\n"
           "address: %s\nport: %d\n", j, addrarr[j].sock_in,
           addrarr[j].sock_out,
           inet_ntoa(addrarr[j].addr.sin_addr),
           htons(addrarr[j].addr.sin_port));
    }
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
        get_time(tb);
        printf("[thread: %ld{%s}] comparing: connected clients[%d]: %s == %s ?\n",
               syscall(__NR_gettid), tb, i, inet_ntoa(ptr->addr.sin_addr), inet_ntoa(lookup.addr.sin_addr));
        if(ptr->addr.sin_addr.s_addr == lookup.addr.sin_addr.s_addr) {
            get_time(tb);
            printf("[thread: %ld {%s}] YES, located at %d returning\n", syscall(__NR_gettid), tb, i);
            return i;
        }
        i++; ptr++;
    }
    get_time(tb);
    printf("[thread: %ld {%s}] not found, returning\n", syscall(__NR_gettid), tb);
    return -1;
}

void *connect_to_file_IPs(void *arg) {

    struct connect_to_file_IPs_args *args = (struct connect_to_file_IPs_args *) arg;
    const char *filename = args->filename;
    char line[31]; // 32 bits for ipv4 address
    in_addr_t conn;

    FILE* file = fopen(filename, "r");
    if((file < 0)) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    while(fgets(line, sizeof(line), file)) {
        // empty line
        if(strncmp(line, "\n", 1) == 0)
            continue;
        conn = inet_addr(line);
        // if host IP skip
        if(conn == args->host_addr.addr.sin_addr.s_addr) {
            get_time(tb);
            printf("[thread: %ld {%s}] Found host IP on list, skipping..\n", syscall(__NR_gettid), tb);
            continue;
        }
        Address new_addr = {0};
        new_addr.addr.sin_addr.s_addr = conn;
        // find empty space in conn_cli array and assign this IPs info to it
        pthread_mutex_lock(args->conn_clients_lock);
        // ===== critical region start ===== //
        if(pconnect(args->conn_clients, args->arrlen, &new_addr) < 0 ) {
            // not added due to already existing, array full or couldn't connect
            get_time(tb);
            printf("[thread: %ld {%s}] Pconnect failed!\n", syscall(__NR_gettid), tb);
        }
        // ===== critical region end ===== //
        pthread_mutex_unlock(args->conn_clients_lock);
    }
    fclose(file);
    pthread_exit(0);
}

int search_IPs(in_addr_t conn, const char *filename) {

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

//static int listen_fds(fd_set *fdset, Address host_addr, Address conn_clients[], int arrlen) {
//
//    int maxsd = 0;
//    FD_ZERO(fdset);
//    FD_SET(host_addr.sock, fdset);
//    for(unsigned int i = 0; i < arrlen; i++) {
//        int tmp = conn_clients[i].sock;
//        if(tmp > 0)
//            FD_SET(tmp, fdset);
//        if(tmp > maxsd)
//            maxsd = tmp;
//    }
//    return maxsd;
//}

static int add_address(Address *arr, Address addr, pthread_mutex_t *arr_lock, int arrlen){

    if(contained_within(arr, addr, arrlen) == 0) {
//        printf("Address already in!\n");
        return -1;
    }
    pthread_mutex_lock(arr_lock);
    int free_space;
    if((free_space = next_space(arr, arrlen)) < 0) {
        printf("Array full!\n");
        return -1;
    }
    arr[free_space] = addr;
    pthread_mutex_unlock(arr_lock);
    printf("Address added\n");
    return 0;
}

static void accept_connection(Address host_addr, Address *client_addr) {

    client_addr->addr_len = sizeof(client_addr->addr);

    if ((client_addr->sock_in = accept(host_addr.sock_in, (struct sockaddr *)&client_addr->addr,
            (socklen_t *)&client_addr->addr_len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    get_time(tb);
    printf("[thread: %ld {%s}] \n"
           "Accepted connection:\nsock: %d\naddress: %s\nport: %d\n",
           syscall(__NR_gettid), tb, client_addr->sock_in,
            inet_ntoa(client_addr->addr.sin_addr),
            htons(client_addr->addr.sin_port));
}

static void add_to_pfds(struct pollfd *pfds, pthread_mutex_t *pfds_lock, int *fdcount, Address add) {
    // need safety check for fdcount
    if(*fdcount > 3)
        exit(EXIT_FAILURE);
    pthread_mutex_lock(pfds_lock);
    pfds[*fdcount].fd = add.sock_in;
    pfds[*fdcount].events = POLLIN;
    (*fdcount)++;
    pthread_mutex_unlock(pfds_lock);
    for(int i = 0; i < *fdcount; i++) {
        get_time(tb);
        printf("[thread: %ld {%s}] pfds[%d].fd = %d\n", syscall(__NR_gettid), tb, i, pfds[i].fd);
    }
}

void *new_connection(void *arg) {

    struct new_connection_args *args = (struct new_connection_args *) arg;
    int loc = 0;
    Address new_client;
    memset(&new_client, 0, sizeof(Address));
    accept_connection(args->host_addr, &new_client);
    // only add addresses from the ipfile

    /*
     * check if its inside conn cli
     * if yes add new socket to the sock_out
     * if no then do rest
     */
    pthread_mutex_lock(args->conn_clients_lock);

//    Address *ptr = args->conn_clients; // Address array
//    int i = 0;
//    int found = 0;
//    while(i < args->arrlen) {
//        get_time(tb);
//        printf("[thread: %ld {%s}] comparing cc %s with new %s\n", syscall(__NR_gettid), tb,
//               inet_ntoa(ptr->addr.sin_addr), inet_ntoa(new_client.addr.sin_addr));
//        if(ptr->addr.sin_addr.s_addr == new_client.addr.sin_addr.s_addr) {
//            get_time(tb);
//            printf("[thread: %ld {%s}] lookup address %s contained within the given array at %d\n",
//                   syscall(__NR_gettid), tb, inet_ntoa(new_client.addr.sin_addr), i);
//            found = 1;
//            break;
//        }
//        i++; ptr++;
//    }

    if((loc = contained_within(args->conn_clients, new_client, args->arrlen)) >= 0) {
        // already in the array
        get_time(tb);
        printf("[thread: %ld {%s}] address already in connected_clients[%d], adding sock_in: %d\n",
               syscall(__NR_gettid), tb, loc, new_client.sock_in);
        args->conn_clients[loc].sock_in = new_client.sock_in;
        add_to_pfds(args->pfds, args->pfds_lock, args->fdcount, new_client);
    } else {
        if (search_IPs(new_client.addr.sin_addr.s_addr, args->filename) == 0) {
            get_time(tb);
            printf("[thread: %ld {%s}] Contained within IP list\n", syscall(__NR_gettid), tb);
            // need a pconnect here, think about logic with the 2 Address sockets
            int free = pconnect(args->conn_clients, args->arrlen, &new_client);
            if (free > -1) {
                get_time(tb);
                printf("[thread: %ld {%s}] Added to conn_clients\n", syscall(__NR_gettid), tb);
                add_to_pfds(args->pfds, args->pfds_lock, args->fdcount, new_client);
                get_time(tb);
                printf("[thread: %ld {%s}] Added to pfds, new size is: %d\n",
                       syscall(__NR_gettid), tb, *args->fdcount);
            }
        }
    }
    pthread_mutex_unlock(args->conn_clients_lock);
    printf("\n");
    for(int j = 0; j < args->arrlen; j++) {
        printf("{accepted} conn_clients[%d]: \nsock_in: %d\nsock_out: %d\n"
               "address: %s\nport: %d\n", j, args->conn_clients[j].sock_in,
               args->conn_clients[j].sock_out,
               inet_ntoa(args->conn_clients[j].addr.sin_addr),
               htons(args->conn_clients[j].addr.sin_port));
    }
    pthread_exit(0);
}

void *server_loop(void *arg) {

    struct server_loop_args *args = (struct server_loop_args *)arg;
    int maxpfds = args->arrlen + 1;// account for host sock
    int fdcount = 1, pollcnt = -99;
    struct pollfd *pfds = malloc(sizeof(*pfds) * maxpfds);
    pthread_mutex_t pfds_lock = PTHREAD_MUTEX_INITIALIZER;
    get_time(tb);
    printf("[thread: %ld {%s}] listening on sock %d\n", syscall(__NR_gettid), tb,
           args->host_addr.sock_in);
    pfds[0].fd = args->host_addr.sock_in;
    pfds[0].events = POLLIN;

    for(;;) {
        get_time(tb);
        printf("[thread: %ld {%s}] back to polling\n", syscall(__NR_gettid), tb);
        if((pollcnt = poll(pfds, fdcount, -1)) < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for(unsigned int i = 0; i < fdcount; i++) {
            if(pfds[i].revents & POLLIN) {
                if(pfds[i].fd == args->host_addr.sock_in) {
                    // if host socket, must be new connection
                    get_time(tb);
                    printf("[thread: %ld {%s}] new incoming connection\n", syscall(__NR_gettid), tb);
                    struct new_connection_args nca = {args->conn_clients, args->conn_clients_lock,
                                           args->host_addr, args->arrlen, &fdcount,
                                           pfds, &pfds_lock, args->filename};
                    pthread_t nc_thread;
//                    pthread_t thisth = syscall(__NR_gettid);
                    pthread_create(&nc_thread, NULL, new_connection, &nca);
                    get_time(tb);
                    printf("[thread: %ld {%s}] fdcount = %d\n", syscall(__NR_gettid), tb, fdcount);
//                    pthread_join(nc_thread, NULL);


//                    printf("\n");
//                    for(int j = 0; j < args->arrlen; j++) {
//                        printf("conn_clients[%d]: \nsock: %d\naddress: %s\nport: %d\n", j, args->conn_clients[j].sock,
//                               inet_ntoa(args->conn_clients[j].addr.sin_addr),
//                               htons(args->conn_clients[j].addr.sin_port));
//                    }
                } else {
                    // reading from already connected client
                    get_time(tb);
                    printf("[thread: %ld {%s}] new message/disconnection\n", syscall(__NR_gettid), tb);
                    char buf[1024] = {0};
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
                    if(nbytes <= 0) {
                        get_time(tb);
                        printf("[thread: %ld {%s}] disconnection\n", syscall(__NR_gettid), tb);
                        close(pfds[i].fd);
                        pfds[i].events = 0;
                        pfds[i].revents = 0;
                        pfds[i].fd = 0;
                        fdcount--;
                    } else {
                        get_time(tb);
                        printf("[thread: %ld {%s}] received: %s\n", syscall(__NR_gettid), tb,
                               buf);
                    }
                }
            }
        }
    } // END of for loop
} // END of server loop

int main(int argc, char *argv[]) {

    if(argc < 4) {
        printf("Not enough arguments given, 4 expected: total-machines port-number interface-name ipfile\n");
        exit(EXIT_FAILURE);
    }
    long nrm = strtol(argv[1], NULL, 10);
    long pnr = strtol(argv[2], NULL, 10);
    const char *infc = argv[3];
    const char *fn = argv[4];
    // Check if returned error from strtol OR if the longs are too large to convert
    if (errno != 0 || ((nrm > INT_MAX) || (pnr > INT_MAX ))) {
        printf("%s argument too large!\n", (nrm > INT_MAX) ? "first" : "second");
        exit(EXIT_FAILURE);
    }
    int err;
    int nrmachines = (int)nrm - 1; // to account for this machine (not adding to connected clients)
    int portnr = (int)pnr;

    Address host_addr = {0,0,0, {0}};
    init_server(&host_addr, nrmachines, portnr, infc);

    // init Address arrays and their corresponding mutex locks
    Address connected_clients[nrmachines];
    memset(connected_clients, 0, sizeof(Address) * (nrmachines));
    pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;;

    Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
    memset(inprog, 0, sizeof(Inprog));
    pthread_mutex_t inprog_lock = PTHREAD_MUTEX_INITIALIZER;

    // connect to IPs in ipfile using main thread 0


    struct server_loop_args sla = {connected_clients, &connected_clients_lock,
                                   inprog, &inprog_lock,
                                   host_addr, nrmachines, fn};
    pthread_t sla_thread;
    get_time(tb);
    printf("[thread: %ld {%s}] running server_loop thread\n", syscall(__NR_gettid), tb);
    pthread_create(&sla_thread, NULL, server_loop, &sla);
//    pthread_join(sla_thread, NULL);

    struct connect_to_file_IPs_args ctipa = {connected_clients, &connected_clients_lock,
                                             host_addr, nrmachines, fn};
    pthread_t ctipa_thread;
    get_time(tb);
    printf("[thread: %ld {%s}] running connect_to_file_IPs thread\n", syscall(__NR_gettid), tb);
    pthread_create(&ctipa_thread, NULL, connect_to_file_IPs, &ctipa);
//    pthread_join(ctipa_thread, NULL);

    sleep(1);
    char buf[1024];
    get_time(tb);
    printf("[thread: %ld {%s}] starting write loop\n", syscall(__NR_gettid), tb);

    for(;;) {
//        if(client_host[0].sock != 0) {
//            printf("Reading\n");
//            if(read(client_host[0].sock, &buf, 1024) < 0) {
//                perror("read");
//                printf("Didn't get anything\n");
//
//            }
//                printf("received: %s\n", buf);
//        }
        char input[32] = {0};
        printf("~ ");
        scanf("%s", input);
        printf("\n");
//        for(int j = 0; j < nrmachines; j++) {
//            printf("{main} conn_clients[%d]: \nsock_in: %d\nsock_out: %d\n"
//                   "address: %s\nport: %d\n", j, connected_clients[j].sock_in,
//                   connected_clients[j].sock_out,
//                   inet_ntoa(connected_clients[j].addr.sin_addr),
//                   htons(connected_clients[j].addr.sin_port));
//        }
            if((err = send(connected_clients[0].sock_out, input, 32, 0)) <= 0) {
//            if((err = write(connected_clients[i].sock_in, input, 32) <= 0)) {
                if(err < 0) {
                    perror("send");
                }
                get_time(tb);
                printf("[thread: %ld {%s}] write to host_client failed\n", syscall(__NR_gettid), tb);
            } else {
                get_time(tb);
                printf("[thread: %ld {%s}] sent %d bytes to %s @ sock_out: %d\n", syscall(__NR_gettid),
                       tb, err, inet_ntoa(connected_clients[0].addr.sin_addr), connected_clients[0].sock_out);
            }

//            printf("host_cli sock[%d]: %d and address: %s\n", i, host_client[i].sock,
//                                                            inet_ntoa(host_client[i].addr.sin_addr));
//            printf("clihost sock[%d]: %d and address: %s\n", i, client_host[i].sock,
//                   inet_ntoa(client_host[i].addr.sin_addr));
    }


// ----- usage of new ds' ------
//    Request *req = (Request *)malloc(sizeof(Request));
//    memset(req, 0, sizeof(Request));
//    Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
//    memset(inprog, 0, sizeof(Inprog));
//    req_tracker_ll_add(&inprog->req_ll_head, req);
//    req_tracker_ll_print(&inprog->req_ll_head);
//    char *tmp = "This is the new buffer content!";
//    if(req_add_content(&inprog->req_ll_head, *req, tmp, strlen(tmp) + 1) < 0) {
//        printf("Unable to find Request\n");
//    }
//    inprog_reset(inprog);

    return 0;
}
