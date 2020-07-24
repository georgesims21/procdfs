#include <string.h>
#include <stdlib.h>
#include "reader.h"
#include "writer.h"

int parse_flag(char *buf) {
    // ASCII magic: https://stackoverflow.com/questions/5029840/convert-char-to-int-in-c-and-c
    int flag = buf[0] - '0';
    memmove(buf, buf + 1, strlen(buf));
    return flag;
}

void parse_path(char *path, char *buf) {

    strncpy(path, buf, MAX_PATH);
    memmove(buf, buf + MAX_PATH, strlen(buf));
}

