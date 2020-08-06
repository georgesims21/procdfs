#ifndef PROCSYS_READER_H
#define PROCSYS_READER_H

int parse_flag(char *message);
void parse_path(char *path, char *buf);
void trim_whitespace(char *buf);
int numPlaces (int n);
void remove_pid(char *buf);
#endif //PROCSYS_READER_H
