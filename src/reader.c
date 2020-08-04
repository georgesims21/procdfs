#include <string.h>
#include <ctype.h>
#include "reader.h"
#include "defs.h"

int parse_flag(char *buf) {
    // ASCII magic: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
    int flag = buf[0] - '0';
    memmove(buf, buf + 1, strlen(buf));
    return flag;
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

