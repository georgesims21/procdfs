#ifndef PROCSYS_WRITER_H
#define PROCSYS_WRITER_H
#include "queue.h"

#define MAX_PATH 64
#define MAX_FLAG 1
void prepend_flag(int flag, char *buf);
void prepend_path(const char *path, char *buf);
void prepend_content(char *content, char *buf);
#endif //PROCSYS_WRITER_H
