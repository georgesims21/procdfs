#include "log.h"
#include "defs.h"
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

//void pprintf(char *message) {
//    FILE *fp;
//    chdir("/home/george/github/procsys");
//
//    if((fp = fopen("procsys.log", "w+")) < 0 ) {
//        perror("fopen");
//        exit(EXIT_FAILURE);
//    }
//
//    fprintf(fp, "%s\n", message);
//    fclose(fp);
//}

void lprintf(const char *fmt, ...) {
//    https://stackoverflow.com/questions/7031116/how-to-create-function-like-printf-variable-argument
    va_list arg;
    FILE *fp;
    chdir("/home/vagrant/");
    fp = fopen("procsys.log", "a+");
    /* Write the error message */
    va_start(arg, fmt);
    vfprintf(fp, fmt, arg);
    va_end(arg);
    fclose(fp);
}
void timestamp_log(char *id) {
    // init log
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    lprintf("\n\n--%s @  %02d/%02d %02d:%02d:%02d--\n", id, tm.tm_mday, tm.tm_mon, tm.tm_hour,
            tm.tm_min, tm.tm_sec);
}