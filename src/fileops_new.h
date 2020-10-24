#ifndef PROCSYS_FILEOPS_NEW_H
#define PROCSYS_FILEOPS_NEW_H

int procnet_dev_extract(char *filebuf, char (*matrix)[32][128]);
int procnet_dev_merge(char (*matrix1)[32][128], char (*matrix2)[32][128], char (*retmatrix)[32][128], int *row_count);
char *mat2buf(char (*matrix)[32][128]);

#endif //PROCSYS_FILEOPS_NEW_H
