#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include "reader.h"
#include "defs.h"

int parse_flag(char *buf) {
    // ASCII magic: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
    int flag = buf[0] - '0';
    memmove(buf, buf + 1, strlen(buf));
    return flag;
}

void remove_pid(char *buf) {

    int pid = getpid();
    size_t pidlen = numPlaces(pid) + 1; // account for leading '/'
    memmove(buf + strlen("/proc"), buf + pidlen + strlen("/proc"), strlen(buf));
}

int numPlaces (int n) {
//    https://stackoverflow.com/questions/1068849/how-do-i-determine-the-number-of-digits-of-an-integer-in-c
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1 - add more ifs as needed
       and adjust this final return as well. */
    return 10;
}

void parse_path(char *path, char *buf) {

    strncpy(path, buf, MAX_PATH);
    memmove(buf, buf + MAX_PATH, strlen(buf));
    trim_whitespace(path);
}

void trim_whitespace(char *buf) {
    // https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
    char *end;
    end = buf + strlen(buf) - 1;
    while(end > buf && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';
}

