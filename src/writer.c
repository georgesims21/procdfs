#include "writer.h"
#include "client-server.h"
#include "defs.h"
#include "log.h"

/*
 * TODO
 *  [ ] document the buffer, first 64bytes for flag and pathname, rest for content
 */

void prepend_flag(int flag, char *buf) {
    // assumes buf is null terminated
    // https://stackoverflow.com/questions/2328182/prepending-to-a-string
    // tried countlessly but it doesn't like + '0' when memmoving

    char fl = flag + '0';
    size_t len = sizeof(fl);
    memmove(buf + len, buf, strlen(buf) + 1);
//    buf[0] = tmp; // feels dirtier than mmove but works
    strncpy(&buf[0], &fl, len);
}

void append_path(const char *path, char *buf, int padding) {

    size_t len = strlen(path);
    snprintf(&buf[padding], len + 1, "%s", path);
}

void append_content(char *content, char *buf, int padding) {

    size_t len = strlen(content);
    snprintf(&buf[padding], len + 1, "%s", content);
}

void fetch_from_server(char *filebuf, const char *fp, char *buf, int flag, int serversock, int pipe) {
    /*
     * TODO
     *  - Find out why heap allocated buffer doesn't allow for string manipulation (like below).
     *    Tried many different approaches: memmove, snprintf with padding, pointers to the buf @
     *    + MAX_PATH + MAX_FLAG and strcpy to those pointers.
     */

    // make space for the buffer: |flag|path|file content|'\0'|
    //               size(bytes):   1    64       n         1
    // where n <= MAX - (MAX_FLAG + MAX_PATH + 1)

    char tmp[MAX] = {0};
    char reply[MAX] = {0};
    size_t flen = 1, fplen = strlen(fp), buflen = strlen(filebuf), total = buflen + MAX_FLAG + MAX_PATH + 1;
    tmp[total] = '\0';

    //flag
    char fl = flag + '0';
    strncpy(&tmp[0], &fl, flen);

    //path
    strncpy(&tmp[MAX_FLAG], fp, fplen);

    //content
    strncpy(&tmp[MAX_FLAG + MAX_PATH], filebuf, buflen);

    write(serversock, tmp, strlen(tmp));
    read(pipe, &reply, sizeof(reply)); // wait for the final file from reader process

    snprintf(buf, strlen(reply), "%s", reply);
}



