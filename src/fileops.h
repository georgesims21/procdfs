#ifndef PROCSYS_FILEOPS_H
#define PROCSYS_FILEOPS_H

#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define MAXPATH 64

static const char *pndpath = "/proc/net/dev";
static char proc[] = "/proc";

int file_exists(const char *pathname);
// All files in /proc are 0 in size, so must access the files and count chars manually
int procsize(const char *pathname);
// Return char data from given file
int datafetch(char *buffer, const char *pathname);
int is_file(const char *path);
int is_dir(const char *path);
mode_t what_am_i(const char *path);
struct stat *retstat(const char *path);
// Given a filepath, fill the buf with its contents
size_t populate(char **buf, size_t size, off_t offset, const char *path);
const char *add_proc(const char *path);

#endif //PROCSYS_FILEOPS_H
