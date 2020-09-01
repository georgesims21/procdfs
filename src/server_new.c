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

#include "server_new.h"
#include "ds_new.h"

int init_server(Address *address, int queue_length, int port_number, const char *interface) {

    if((address->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(address->sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
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

    if(bind(address->sock, (struct sockaddr*)&address->addr, address->addr_len) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(address->sock, queue_length);
    printf("Listening on :%s at port: %d\n", inet_ntoa(address->addr.sin_addr), htons(address->addr.sin_port));
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

static int pconnect(Address *addrarr, int arrlen, in_addr_t conn) {
    int free = 0;
    Address tmp;
    tmp.addr.sin_family = AF_INET;
    tmp.addr.sin_port = htons(1234); // using generic port here, find solution
    tmp.addr.sin_addr.s_addr = conn;
    tmp.addr_len = sizeof(tmp.addr);
    printf("checking if client is inside the host_client array\n");
    if(contained_within(addrarr, tmp, arrlen) == 0) {
        printf("Address already inside array\n");
        return -1;
    }
    free = next_space(addrarr, arrlen);
    if(free < 0) {
        printf("Maximum number of machines reached (%d), not adding!\n", arrlen);
        return -1;
    }

    if((tmp.sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }
    if(connect(tmp.sock, (struct sockaddr*)&tmp.addr, tmp.addr_len) < 0) {
        printf("Cannot connect to %d \n", tmp.sock);
        return -2;
    }

    Address *ptr = &addrarr[free]; // Address array
    *ptr = tmp;

    printf("Added %s to list at %d and connected.. sending message\n", inet_ntoa(ptr->addr.sin_addr), free);
//    write(ptr->sock, "Welcome to the server", 22);
    return free;
}

static int next_space(Address *addr, int arrlen) {
    // assumes user has locked addr
    int i = 0;
    Address *ptr = addr; // Address array
    while(i < arrlen) {
        if(ptr->sock == 0) {
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
        if(
//                ptr->sock == lookup.sock &&
//           ptr->addr_len == lookup.addr_len &&
//           ptr->addr.sin_family == lookup.addr.sin_family
           ptr->addr.sin_addr.s_addr == lookup.addr.sin_addr.s_addr &&
           ptr->addr.sin_port == lookup.addr.sin_port
           ) {
            printf("lookup address %s contained within the given array at %d\n",
                   inet_ntoa(lookup.addr.sin_addr), i);
            return 0;
        }
        i++; ptr++;
    }
    printf("lookup address %s NOT contained within the given array\n",
           inet_ntoa(lookup.addr.sin_addr));
    return -1;
}

void *connect_to_file_IPs(void *arg) {

    struct connect_to_file_IPs_args *args = (struct connect_to_file_IPs_args *) arg;
    const char *filename = args->filename;
    char line[32]; // 32 bits for ipv4 address
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
            printf("Found host IP on list, skipping..\n");
            continue;
        }
        // find empty space in host-cli array and assign this IPs info to it
        pthread_mutex_lock(args->host_client_lock);
        // start of critical region 1
        int hcarrloc = pconnect(args->host_client, args->arrlen, conn);
        if(hcarrloc < 0 ) {
            // not added due to already existing, array full or couldn't connect
            continue;
        }
        Address newaddr = args->host_client[hcarrloc];
        // end of critical region 1
        pthread_mutex_unlock(args->host_client_lock);

        pthread_mutex_lock(args->client_host_lock);
        // start of critical region 2
        printf("Checking if:\nsock: %d\naddress: %s\nport: %d\nis inside the client_host array\n", newaddr.sock,
                                                            inet_ntoa(newaddr.addr.sin_addr),
                                                            htons(newaddr.addr.sin_port));
        if(contained_within(args->client_host, newaddr, args->arrlen) != 0) {
            printf("Not contained within client_host array, not adding to conn_clients\n");
            pthread_mutex_unlock(args->client_host_lock);
            continue;
        }
        int charrloc = next_space(args->client_host, args->arrlen);
        // end of critical region 2
        pthread_mutex_unlock(args->client_host_lock);

        pthread_mutex_lock(args->conn_clients_lock);
        // start of critical region 3
        int ccarrloc = next_space(args->conn_clients, args->arrlen);
        printf("Checking if:\nsock: %d\naddress: %s\nport: %d\nis inside the conn_clients array\n", newaddr.sock,
               inet_ntoa(newaddr.addr.sin_addr),
               htons(newaddr.addr.sin_port));
        if(contained_within(args->conn_clients, newaddr, args->arrlen) == 0) {
            printf("already inside conn_clients\n");
            pthread_mutex_unlock(args->conn_clients_lock);
            continue;
        }
        args->conn_clients[ccarrloc] = newaddr; // is this safe?
        // end of critical region 3
        pthread_mutex_unlock(args->conn_clients_lock);
    }
    fclose(file);
    pthread_exit(0);
}

int search_IPs(in_addr_t conn, char *filename) {

    char line[32]; // 32 bits for ipv4 address
    in_addr_t newconn;
    FILE* file = fopen(filename, "r");
    if((file < 0)) {
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
static int listen_fds(fd_set *fdset, Address host_addr, Address conn_clients[], int arrlen) {

    int maxsd = 0;
    FD_ZERO(fdset);
    FD_SET(host_addr.sock, fdset);
    for(unsigned int i = 0; i < arrlen; i++) {
        int tmp = conn_clients[i].sock;
        if(tmp > 0)
            FD_SET(tmp, fdset);
        if(tmp > maxsd)
            maxsd = tmp;
    }
    return maxsd;
}

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

    if ((client_addr->sock = accept(host_addr.sock, (struct sockaddr *)&client_addr->addr,
            (socklen_t *)&client_addr->addr_len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Accepted connection:\nsock: %d\naddress: %s\nport: %d\n", client_addr->sock,
           inet_ntoa(client_addr->addr.sin_addr),
           htons(client_addr->addr.sin_port));

}

static void add_to_pfds(struct pollfd *pfds, pthread_mutex_t *pfds_lock, int *fdcount, Address add) {
    // need safety check for fdcount
    pthread_mutex_lock(pfds_lock);
    pfds[*fdcount].fd = add.sock;
    pfds[*fdcount].events = POLLIN;
    (*fdcount)++;
    pthread_mutex_unlock(pfds_lock);
}

void *new_connection(void *arg) {

    struct new_connection_args *args = (struct new_connection_args *) arg;
    Address new_client;
    memset(&new_client, 0, sizeof(Address));
    accept_connection(args->host_addr, &new_client);
    // add new connection to client_host array
    add_address(args->client_host, new_client, args->client_host_lock, args->arrlen);
    add_to_pfds(args->pfds, args->pfds_lock, args->fdcount, new_client);
    // if also in hostclient array, add to connected clients
    printf("Checking if:\nsock: %d\naddress: %s\nport: %d\nis inside the host_client array after adding"
           "to the client_host\n", new_client.sock,
           inet_ntoa(new_client.addr.sin_addr),
           htons(new_client.addr.sin_port));
    if(contained_within(args->host_client, new_client, args->arrlen) < 0) {
        printf("Not contained within host client array\n");
        // check if IP matches one from file, if yes pconnect
       if(search_IPs(new_client.addr.sin_addr.s_addr, "iplist.txt") == 0) {
           printf("IP is contained within the iplist file, safe to add\n");
           if(pconnect(args->host_client, args->arrlen, new_client.addr.sin_addr.s_addr) < 0) {
               printf("pconnect failed, not adding to connected clients array\n");
           } else {
               add_address(args->conn_clients, new_client, args->conn_clients_lock, args->arrlen);
               printf("Client:\nsock: %d\naddress: %s\nport: %d\n"
                      "is now inside the conn_client array\n", new_client.sock,
                      inet_ntoa(new_client.addr.sin_addr),
                      htons(new_client.addr.sin_port));
           }
       }
    }
    pthread_exit(0);
}

void *server_loop(void *arg) {

    struct server_loop_args *args = (struct server_loop_args *)arg;
    int maxpfds = args->arrlen + 1;
    struct pollfd *pfds = malloc(sizeof(*pfds) * maxpfds); // account for host sock
    pthread_mutex_t pfds_lock = PTHREAD_MUTEX_INITIALIZER;
    pfds[0].fd = args->host_addr.sock;
    pfds[0].events = POLLIN;
    int fdcount = 1;

    for(;;) {
        printf("back to polling\n");
        if(poll(pfds, fdcount, -1)) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for(unsigned int i = 0; i < fdcount; i++) {
            if(pfds[i].revents & POLLIN) {
                if(pfds[i].fd == args->host_addr.sock) {
                    // if host socket, must be new connection
                    printf("New connection\n");
                    struct new_connection_args nca = {args->conn_clients, args->conn_clients_lock,
                                           args->host_client, args->host_client_lock,
                                           args->client_host, args->client_host_lock,
                                           args->host_addr, args->arrlen, &fdcount,
                                           pfds, &pfds_lock};
                    pthread_t nc_thread;
                    pthread_create(&nc_thread, NULL, new_connection, &nca);
                    pthread_join(nc_thread, NULL);

//                    printf("\n");
//                    for(int j = 0; j < args->arrlen; j++) {
//                        printf("host_client[%d]: \nsock: %d\naddress: %s\nport: %d\n", j, args->host_client[j].sock,
//                               inet_ntoa(args->host_client[j].addr.sin_addr),
//                               htons(args->host_client[j].addr.sin_port));
//                    }
//                    printf("\n");
//                    for(int j = 0; j < args->arrlen; j++) {
//                        printf("client_host[%d]: \nsock: %d\naddress: %s\nport: %d\n", j, args->client_host[j].sock,
//                               inet_ntoa(args->client_host[j].addr.sin_addr),
//                               htons(args->client_host[j].addr.sin_port));
//                    }
//                    printf("\n");
//                    for(int j = 0; j < args->arrlen; j++) {
//                        printf("conn_clients[%d]: \nsock: %d\naddress: %s\nport: %d\n", j, args->conn_clients[j].sock,
//                               inet_ntoa(args->conn_clients[j].addr.sin_addr),
//                               htons(args->conn_clients[j].addr.sin_port));
//                    }
                } else {
                    // reading from already connected client
                    printf("New message/disconnection\n");
                    char buf[1024] = {0};
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
                    if(nbytes <= 0) {
                        printf("Disconnection\n");
                        close(pfds[i].fd);
                        pfds[i].events = 0;
                        pfds[i].revents = 0;
                        pfds[i].fd = 0;
                        fdcount--;
                    } else {
                        printf("Received: %s\n", buf);
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
    int nrmachines = (int)nrm - 1; // to account for this machine (not adding to connected clients)
    int portnr = (int)pnr;

    Address host_addr = {0, 0, {0}};
    init_server(&host_addr, nrmachines, portnr, infc);

    // init Address arrays and their corresponding mutex locks
    Address connected_clients[nrmachines];
    memset(connected_clients, 0, sizeof(Address) * (nrmachines));
    pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;;

//    memcpy(&connected_clients, &host_addr, sizeof(Address));

    Address host_client[nrmachines];
    memset(host_client, 0, sizeof(Address) * (nrmachines));
    pthread_mutex_t host_client_lock = PTHREAD_MUTEX_INITIALIZER;

    Address client_host[nrmachines];
    memset(client_host, 0, sizeof(Address) * (nrmachines));
    pthread_mutex_t client_host_lock = PTHREAD_MUTEX_INITIALIZER;

    Inprog *inprog = (Inprog *)malloc(sizeof(Inprog));
    memset(inprog, 0, sizeof(Inprog));
    pthread_mutex_t inprog_lock = PTHREAD_MUTEX_INITIALIZER;

//    for(int i = 0; i < 1; i++) {
//        host_client[i].sock = i + 1;
//    }

    // connect to IPs in ipfile using main thread 0
    struct connect_to_file_IPs_args ctipa = {connected_clients, &connected_clients_lock,
                                        host_client, &host_client_lock,
                                        client_host, &client_host_lock,
                                        host_addr, nrmachines, fn};
    pthread_t ctipa_thread;
    pthread_create(&ctipa_thread, NULL, connect_to_file_IPs, &ctipa);
    pthread_join(ctipa_thread, NULL);

    struct server_loop_args sla = {connected_clients, &connected_clients_lock,
                                   host_client, &host_client_lock,
                                   client_host, &client_host_lock,
                                   inprog, &inprog_lock,
                                   host_addr, nrmachines};
    pthread_t sla_thread;
    pthread_create(&sla_thread, NULL, server_loop, &sla);
//    pthread_join(sla_thread, NULL);

    sleep(1);
    char buf[1024];
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
        for(int i = 0; i < nrmachines; i++) {
            if(write(host_client[i].sock, input, 32) <= 0) {
                printf("Write to host_client failed!\n");
            } else {
                printf("Sent to host_client\n");
            }
            printf("host_cli sock[%d]: %d and address: %s\n", i, host_client[i].sock,
                                                            inet_ntoa(host_client[i].addr.sin_addr));
            printf("clihost sock[%d]: %d and address: %s\n", i, client_host[i].sock,
                   inet_ntoa(client_host[i].addr.sin_addr));
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
