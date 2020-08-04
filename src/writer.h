#ifndef PROCSYS_WRITER_H
#define PROCSYS_WRITER_H
#include "queue.h"


void prepend_flag(int flag, char *buf);
void append_path(const char *path, char *buf, int padding);
void append_content(char *content, char *buf, int padding);
void fetch_from_server(char *filebuf, const char *fp, char *buf, int flag, int serversock, int pipe);
#endif //PROCSYS_WRITER_H
