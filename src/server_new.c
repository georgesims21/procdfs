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
                char buf[1024] = {0};
                int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0); // needs doing correctly
                if(nbytes <= 0) {
                    get_time(tb);
                    printf("[thread: %ld {%s}] disconnection, exiting..\n", syscall(__NR_gettid), tb);
                    exit(EXIT_SUCCESS);
                } else {
                    get_time(tb);
                    printf("[thread: %ld {%s}] (%d) bytes received: %s\n", syscall(__NR_gettid), tb,
                           nbytes, buf);
                }
            }
        } // END of poll loop
    } // END of inf for loop
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

    printf("Connecting to other machines..\n");
    Address host_addr;
    memset(&host_addr, 0, sizeof(host_addr));
    init_server(&host_addr, nrmachines, portnr, infc);

    // init Address arrays and their corresponding mutex locks
    Address connected_clients[nrmachines];
    memset(connected_clients, 0, sizeof(Address) * (nrmachines));
    pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;;

    Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
    memset(inprog, 0, sizeof(Inprog));
    pthread_mutex_t inprog_lock = PTHREAD_MUTEX_INITIALIZER;

    // accept all incoming connections until have sock_in for all machines in list
    struct accept_connection_args aca = {connected_clients, &connected_clients_lock,
                                          host_addr, nrmachines, fn};
    pthread_t aca_thread;
    pthread_create(&aca_thread, NULL, accept_connection, &aca);
    // connect to IPs in ipfile
    struct connect_to_file_IPs_args ctipa = {connected_clients, &connected_clients_lock,
                                             host_addr, nrmachines, fn};
    pthread_t ctipa_thread;
    pthread_create(&ctipa_thread, NULL, connect_to_file_IPs, &ctipa);
    // force main to wait until connected to all machines
    pthread_join(ctipa_thread, NULL);
    pthread_join(aca_thread, NULL);

    printf("You are now connected to machines: \n");
    for(int j = 0; j < nrmachines; j++) {
        printf("%s\t@\t%d\n",
               inet_ntoa(connected_clients[j].addr.sin_addr),
               htons(connected_clients[j].addr.sin_port));
    }

    // start server loop to listen for connections
    struct server_loop_args sla = {connected_clients, &connected_clients_lock,
                                   inprog, &inprog_lock,
                                   host_addr, nrmachines, fn};
    pthread_t sla_thread;
    pthread_create(&sla_thread, NULL, server_loop, &sla);
    char buf[1024];
    for(;;) {
        printf("~ ");
        scanf("%s", buf);
        for(int i = 0; i < nrmachines; i++) {
            if((err = send(connected_clients[i].sock_out, buf, sizeof(buf), 0)) <= 0) {
                if(err < 0) {
                    perror("send");
                }
                get_time(tb);
                printf("[thread: %ld {%s}] write to host_client failed\n", syscall(__NR_gettid), tb);
            } else {
                get_time(tb);
                printf("[thread: %ld {%s}] sent %d bytes to %s @ sock_out: %d\n", syscall(__NR_gettid),
                       tb, err, inet_ntoa(connected_clients[i].addr.sin_addr), connected_clients[i].sock_out);
            }
        }
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
