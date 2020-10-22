#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fileops_new.h"


int procnet_dev_extract(char *filebuf, char (*matrix)[32][128]) {

    char tmp[128] = {0}, *c = filebuf;
    int count = 0, i = 0, j = 0, k = 0, nl = 0;

    while(*c != '\0') {
        while(strncmp(c, " ", 1) != 0) {
            strncat(tmp, c, 1);
            count++;
            if(*c == '\n') {
                strncpy(matrix[i][j], tmp, strlen(tmp) - 1);
                memset(tmp, 0, 128);
                j++;
                strncpy(matrix[i][j], c, 1); // save newline as last index in row avoiding merging errors
                i++; j = 0; c++; count = 0;
                if(strncmp(c, " ", 1) == 0)
                    nl = 1; // allows for including spaces before string (after newline)
                continue;
            } else if(*c == '\0') {
                strncpy(matrix[i][j], tmp, sizeof(tmp));
                return 0;
            } else if(i == 1 && j == 8) { // to account for [1][8] problematic '|' delim
                if(strncmp(c, "|", 1) == 0) {
                    c++;
                    break;
                }
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

int procnet_dev_merge(char (*matrix1)[32][128], char (*matrix2)[32][128], char (*retmatrix)[32][128]) {

    /*
     * keep in mind that machines may not share the same interfaces so if it doesn't exist on both,
     * still save it but on a different line. If both share it then merge the whole row (may be on different
     * rows)
     *
         * rows 0 and 1 are reserved, 2 is where interfaces start ([2][0] == first interface)
         * Column 0 is reserved on each row for the name
         * Will always be 18 columns - can use 2d array to save unmatched interfaces and write these
         * to the final 3d array at the end
     */
    char row[32][128] = {0};
    char matrix3[32][32][128] = {0};

    // Fill in first 2 rows with header info
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 32; j++) {
            memcpy(retmatrix[i][j], matrix1[i][j], strlen(matrix1[i][j]));
        }
    }

    // need to copy rows 1 and 2 to matrix 3
    int row_count = 2; // to keep consistent rows for new matrix (or could be row 1, 3, 7 have text etc)
    for(int i = 2; i < 32; i++) { // matrix1
        // get one value from here, need to save it into matrix 3
        for(int ii = 2; ii < 32; ii++) { // matrix2
            // check all of values in here against one from matrix 1
            if(strncmp(matrix1[i][0], matrix2[ii][0], strlen(matrix1[i][0])) == 0) {
                // if they do match, merge the values in row: need to add [i][0] to matrix3
                for(int j = 1; j < 32; j++) { // matrix2, row iterator
                    long val1 = strtol(matrix1[i][j], NULL, 10);
                    long val2 = strtol(matrix2[ii][j], NULL, 10);
                    long final = val1 + val2;
                    /* Here need to use formatting from original (matrix1[i][j] or 2[ii][j]) string
                       need to save everything except the numbers themselves*/
                    snprintf(retmatrix[row_count][j], sizeof(final), "%lu", final);
                }
                row_count++;
                break; // should break out of matrix 2 loop
            }
            if(ii++ == 32) {
                // not found,
            }
        }
    }
//    for(int i = 2; i < 32; i++) {
//        for(int j = 1; j < 32; j++) {
//
//        }
//    }
    return 0;
}
