#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

int extractor(char *string, char *before, char *digit, char *after) {

    if(!string) {
        printf("Given empty string!\n");
        exit(EXIT_FAILURE);
    }
    if(strncmp(string, "", 1) == 0) {
        return 2;
    }
    size_t len = strlen(string);
    char *c = string;
    if(*c == '\n') {
        return 1;
    }
    int count = 0;
    while(!(*c >= '0' && *c <= '9')) {
        // 'before'
        strncat(before, c, 1);
        c++; count++;
    }
    while(*c >= '0' && *c <= '9') {
        // 'digit'
        strncat(digit, c, 1);
        c++; count++;
    }
//    size_t len = strlen(string) - strlen(before) - strlen(after);
    memcpy(after, &string[count], strlen(string) - count);
    after[len] = '\0';
    return 0;
}

int procnet_dev_merge(char (*matrix1)[32][128], char (*matrix2)[32][128], char (*retmatrix)[32][128], int *row_count) {

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
    bool add_row = false;

    if(*row_count == 0) {
        // first run/merge so add header info
        for(int i = 0; i < 2; i++) {
            for(int j = 0; j < 32; j++) {
                memcpy(retmatrix[i][j], matrix1[i][j], strlen(matrix1[i][j]));
            }
            (*row_count)++;
        }
    }

    // need to copy rows 1 and 2 to matrix 3
    for(int i = 2; i < 32; i++) { // matrix1
        if(strcmp(matrix1[i][0], "") == 0) {
            // to avoid overread
            break;
        }
        // get one value from here, need to save it into matrix 3
        for(int ii = 2; ii < 32; ii++) { // matrix2
            if(strcmp(matrix2[ii][0], "") == 0) {
                add_row = true;
                break;
            }
            // check all of values in here against one from matrix 1
            printf("matrix1[%d][0]: \"%s\"  -  matrix2[%d][0]: \"%s\"\n", i, matrix1[i][0], ii, matrix2[ii][0]);
//            if(strncmp(matrix1[i][0], matrix2[ii][0], strlen(matrix1[i][0])) == 0) {
            if(strcmp(matrix1[i][0], matrix2[ii][0]) == 0) {
                printf("Matching\n");
                // copy interface name in first index
                memcpy(retmatrix[*row_count][0], matrix1[i][0], strlen(matrix1[i][0]));
                // go through indexes replacing string with merged counterpart
                for(int j = 1; j <= 17; j++) { // matrix2, row iterator
                    char m1_before[128] = {0};
                    char m1_digit[128] = {0};
                    char m1_after[128] = {0};
                    char m2_before[128] = {0};
                    char m2_digit[128] = {0};
                    char m2_after[128] = {0};
                    int ext = 0;

                    if((ext = extractor(matrix1[i][j], m1_before, m1_digit, m1_after) == 1)) {
                        // new line, need to skip calculations
                        strcpy(retmatrix[*row_count][j], "\n");
                        goto skip_calc;
                    } else if(ext == 2) {
                        break;
                    }
                    if(extractor(matrix2[ii][j], m2_before, m2_digit, m2_after) == 1) {
                        // if newline was found, first conditional should see it
                        goto skip_calc;
                    }
                    long val1 = strtol(m1_digit, NULL, 10);
                    long val2 = strtol(m2_digit, NULL, 10);
                    long final = val1 + val2;
                    /* Here need to use formatting from original (matrix1[i][j] or 2[ii][j]) string
                       need to save everything except the numbers themselves*/
                    snprintf(retmatrix[*row_count][j], strlen(m1_before) + sizeof(long) + strlen(m1_after), "%s%lu%s", m1_before, final, m1_after);
                }
                skip_calc:
                (*row_count)++;
                break;
            }
            printf("not matching\n");
        }
        // here the row wasn't found in matrix2
        if(add_row) {
            for(int j = 0; j <= 17; j++) {
                memcpy(retmatrix[*row_count][j], matrix1[i][j], strlen(matrix1[i][j]));
            }
            (*row_count)++;
        }
    }
    return 0;
}
