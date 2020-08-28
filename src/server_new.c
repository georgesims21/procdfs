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
    printf("Listening on :%u\n", address->addr.sin_addr.s_addr);
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
    if(contained_within(addrarr, tmp, arrlen) == 0) {
        printf("Address already inside array");
        return -1;
    }
    free = next_space(addrarr, arrlen);
    if(free < 0) {
        printf("Maximum number of machines reached (%d), not adding!\n", arrlen);
        return -1;
    }

    Address *ptr = &addrarr[free]; // Address array
    ptr->addr.sin_family = AF_INET;
    ptr->addr.sin_port = htons(1234); // using generic port here, find solution
    ptr->addr.sin_addr.s_addr = conn;
    ptr->addr_len = sizeof(ptr->addr);

    if((ptr->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }
    if(connect(ptr->sock, (struct sockaddr*)&ptr->addr, ptr->addr_len) < 0) {
    	printf("Cannot connect to %d \n", ptr->sock);
    	return -2;
    }
    printf("Added %u to list at %d and connected\n", ptr->addr.sin_addr.s_addr, free);
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
        if(ptr->sock == lookup.sock &&
           ptr->addr_len == lookup.addr_len &&
           ptr->addr.sin_addr.s_addr == lookup.addr.sin_addr.s_addr &&
           ptr->addr.sin_port == lookup.addr.sin_port &&
           ptr->addr.sin_family == lookup.addr.sin_family)
            return 0;
        i++; ptr++;
    }
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
            // host is included in max machines count
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
        printf("Address already in!\n");
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
    printf("Accepted connection\n");
}

void *new_connection(void *arg) {

    struct accept_connection_args *args = (struct accept_connection_args *) arg;
    Address new_client;
    memset(&new_client, 0, sizeof(Address));
    accept_connection(args->host_addr, &new_client);
    // add new connection to client_host array
    add_address(args->client_host, new_client, args->client_host_lock, args->arrlen);
    // if also in hostclient array, add to connected clients
    if(contained_within(args->host_client, new_client, args->arrlen) < 0) {
        printf("Not contained within host client array\n");
        // check if IP matches one from file, if yes pconnect
       if(search_IPs(new_client.addr.sin_addr.s_addr, "iplist.txt") == 0) {
           printf("IP is contained within the iplist file, safe to add\n");
           pconnect(args->host_client, args->arrlen, new_client.addr.sin_addr.s_addr);
       }
    }
    add_address(args->conn_clients, new_client, args->conn_clients_lock, args->arrlen);
    printf("Client :%u has been added to connected clients array!\n", new_client.addr.sin_addr.s_addr);
    pthread_exit(0);
}

void *server_loop(void *arg) {

    struct server_loop_args *args = (struct server_loop_args *)arg;
    fd_set fdset;
    int sd = 0, maxsd = 0;

    for(;;) {
        // dirty reads not important because next loop we will add them instead
        maxsd = listen_fds(&fdset, args->host_addr, args->conn_clients, args->arrlen);
        if (FD_ISSET(args->host_addr.sock, &fdset)) {
            // new connection
            struct accept_connection_args aca = {args->conn_clients, args->conn_clients_lock,
                                           args->host_client, args->host_client_lock,
                                           args->client_host, args->client_host_lock,
                                           args->host_addr, args->arrlen};
            pthread_t aca_thread;
            pthread_create(&aca_thread, NULL, new_connection, &aca);
            pthread_join(aca_thread, NULL);
        }
        for(unsigned int i = 0; i < args->arrlen; i++) {
            sd = args->conn_clients[i].sock;
            if(FD_ISSET(sd, &fdset)) {
                // else something written by a connected client
                printf("Something was written/disconnected\n");
                char buf[15] = {0};
                if ((read(sd, buf, 1)) == 0) {
                    //Somebody disconnected
                    printf("Client disconnected!\n");
                } else {
                    printf("Buf receieved: %s\n", buf);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {

    if(argc < 4) {
        printf("Not enough arguments given, 3 expected: total-machines port-number interface-name ipfile\n");
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
    for(;;) {
        char input[32] = {0};
        printf("~ ");
        scanf("%s", input);
        write(connected_clients[0].sock, input, 32);
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
