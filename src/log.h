#ifndef PROCSYS_LOG_H
#define PROCSYS_LOG_H

void pprintf(char *message);
void lprintf(const char *fmt, ...);
void timestamp_log(char *id);

#endif //PROCSYS_LOG_H
