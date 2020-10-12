#include <string.h>
#include "fileops_new.h"


int procnet_dev_op(char *filebuf, char (*matrix)[128][128]) {

    char tmp[64] = {0}, *c = filebuf;
    int len = (int)strlen(filebuf), count = 0, i = 0, j = 0, k = 0, nl = 0;

    while(*c != '\0') {
        while(strncmp(c, " ", 1) != 0) {
            strncat(tmp, c, 1);
            count++;
            if(*c == '\n') {
                strncpy(matrix[i][j], tmp, sizeof(tmp));
                memset(tmp, 0, 64);
                i++; j = 0; c++; count = 0;
                if(strncmp(c, " ", 1) == 0)
                    nl = 1;
                continue;
            } else if(*c == '\0') {
                strncpy(matrix[i][j], tmp, sizeof(tmp));
                return 0;
            }
            c++;
        }
        if(strncmp(c, " ", 1) == 0) {
            // save all whitespace between strings in one index
            int pre_count = count;
            while(strncmp(c, " ", 1) == 0) {
                c++; count++;
            }
            memset(&tmp[pre_count], ' ', count - pre_count);
            tmp[count] = '\0';
            if(nl) {
                nl = 0;
                continue;
            }
        }
        strncpy(matrix[i][j], tmp, sizeof(tmp));
        memset(tmp, 0, 64);
        j++; count = 0;
    }
    return 0;
}
