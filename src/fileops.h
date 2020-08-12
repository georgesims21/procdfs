#ifndef PROCSYS_FILEOPS_H
#define PROCSYS_FILEOPS_H

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

static char proc[] = "/proc";

int file_exists(const char *pathname);
int procsizefd(int fd);
// All files in /proc are 0 in size, so must access the files and count chars manually
int procsize(const char *pathname);
// Return char data from given file
int datafetch(char *buffer, const char *pathname);
mode_t what_am_i(const char *path);
struct stat *retstat(const char *path);
// Given a filepath, fill the buf with its contents
size_t populate(char **buf, size_t size, off_t offset, const char *path);
void final_path(const char *path, char *buf);
char **dir_contents(const char *path);
int dir_size(const char *path);
int parse_message(char *message);
#endif //PROCSYS_FILEOPS_H
