#include "server_new.h"

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

void print_ip(Address *addr) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            if(strcmp(ifa->ifa_name, "vboxnet1") == 0) {
//                 addr->addr = (struct sockaddr_in *)ifa->ifa_addr;
            }

//            printf("<Interface>: %s \t <Address> %s\n", ifa->ifa_name, host);
        }
    }
}


int init_server(Address *address, int queue_length, int port_number) {

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
    address->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    address->addr_len = sizeof(address->addr);

    if(bind(address->sock, (struct sockaddr*)&address->addr, address->addr_len) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(address->sock, queue_length);
    return 0;
}

void *connect_to_IPs(void *arg) {

    struct connect_to_IPs_args *args = (struct connect_to_IPs_args *) arg;
    Address *cliho = args->client_host;
    char const* const fileName = "iplist.txt"; /* should check that argc > 1 */
    char line[32];

    FILE* file = fopen(fileName, "r"); /* should check the result */
    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        printf("%s", line);
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {

    if(argc < 3) {
        printf("Not enough arguments given, 2 expected. Please give the maximum "
               "number of machines on the network, the port number.\n");
        exit(EXIT_FAILURE);
    }
    long nrm = strtol(argv[1], NULL, 10);
    long pnr = strtol(argv[2], NULL, 10);
    // Check if returned error from strtol OR if the longs are too large to convert
    if (errno != 0 || ((nrm > INT_MAX) || (pnr > INT_MAX ))) {
        printf("%s argument too large!\n", (nrm > INT_MAX) ? "first" : "second");
        exit(EXIT_FAILURE);
    }
    int nrmachines = (int)nrm;
    int portnr = (int)pnr;

    Address host_addr = {0, 0, {0}};

    init_server(&host_addr, nrmachines, portnr);

    // init Address arrays and their corresponding mutex locks
    Address connected_clients[nrmachines - 1];
    memset(connected_clients, 0, sizeof(Address) * (nrmachines - 1));
    pthread_mutex_t connected_clients_lock = PTHREAD_MUTEX_INITIALIZER;;

//    memcpy(&connected_clients, &host_addr, sizeof(Address));

    Address host_client[nrmachines - 1];
    memset(host_client, 0, sizeof(Address) * (nrmachines - 1));
    pthread_mutex_t host_client_lock = PTHREAD_MUTEX_INITIALIZER;

    Address client_host[nrmachines - 1];
    memset(client_host, 0, sizeof(Address) * (nrmachines - 1));
    pthread_mutex_t client_host_lock = PTHREAD_MUTEX_INITIALIZER;


    // connect to IPs in ipfile using main thread 0
    struct connect_to_IPs_args ctipa = {connected_clients, &connected_clients_lock,
                                        host_client, &host_client_lock,
                                        client_host, host_addr, nrmachines};
    pthread_t tid;
    pthread_create(&tid, NULL, connect_to_IPs, &ctipa);
    pthread_join(tid, NULL);
    return 0;
}