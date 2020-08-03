#ifndef PROCSYS_WRITER_H
#define PROCSYS_WRITER_H
#include "queue.h"

#define MAX_PATH 63
#define MAX_FLAG 1
void prepend_flag(int flag, char *buf);
void append_path(const char *path, char *buf, int padding);
void append_content(char *content, char *buf, int padding);
#endif //PROCSYS_WRITER_H
