#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

void bprintf(const char *fmt, ...) {
//    https://stackoverflow.com/questions/7031116/how-to-create-function-like-printf-variable-argument
    va_list arg;
    char logfile[128];
    FILE *logfp;
    //snprintf(logfile, 128, "%s%s", "/home/gss680/benchmarks/procfs/LOG", inet_ntoa(host_addr.addr.sin_addr));
    if(!logfp) {
        if((logfp = fopen("/home/gss680/benchmarks/procfs/LOG", "a+")) < 0) {
            printf("Couldn't find log file, no IP matches host\n");
            exit(EXIT_FAILURE);
        }
        if(!logfp) {
            perror("fopen logfile");
            exit(EXIT_FAILURE);
        }
    }
    /* Write the error message */
    va_start(arg, fmt);
    vfprintf(logfp, fmt, arg);
    va_end(arg);
}

char *files[6] = {
    "/proc/net/rt6_stats",
    "/proc/net/raw6",
    "/proc/net/igmp",
    "/proc/net/dev",
    "/proc/net/netstat",
    "/proc/net/ipv6_route"
    };

int main (int argc, char *argv[]) {

    clock_t start, end;
    double cpu_time_used;
    int N = 15000;
    for(int ii = 0; ii < 6; ii++) {
        char cmd[64];
        //snprintf(cmd, 64, "cat %s >/dev/null 2>&1", files[ii]);
        snprintf(cmd, 64, "cat %s", files[ii]);
        start = clock();
        for(int i = 0; i < N; i++) {
            FILE *pf;
            char command[20];
            char data[512];

            // Execute a process listing
            sprintf(command, "cat /proc/net/dev"); 
            pclose(pf);
        }
        end = clock();
        cpu_time_used = (((double) (end - start)) / CLOCKS_PER_SEC) / N;
        bprintf("%s took %f\n", files[ii], cpu_time_used);
    }
    return 0;
}
