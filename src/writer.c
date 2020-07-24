#include "writer.h"

/*
 * TODO
 *  [ ] document the buffer, first 64bytes for flag and pathname, rest for content
 */

void prepend_flag(int flag, char *buf) {
    // assumes buf is null terminated
    // https://stackoverflow.com/questions/2328182/prepending-to-a-string
    // tried countlessly but it doesn't like + '0' when memmoving

    char tmp = flag + '0';
    size_t len = sizeof(tmp);
    memmove(buf + len, buf, strlen(buf) + 1);
    *buf = tmp; // feels dirtier than mmove but works
}

void prepend_path(const char *path, char *buf) {

    size_t len = strlen(path);
    memmove(buf + MAX_PATH, buf, strlen(buf) + 1);
    memcpy(buf, path, len);
//    *buf = path; // feels dirtier than mmove but works
}

void prepend_content(char *content, char *buf) {

    size_t len = strlen(content);
    memmove(buf + MAX_PATH + MAX_FLAG, buf, strlen(buf) + 1);
    memcpy(buf, content, len);
}



