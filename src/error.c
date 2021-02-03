#include <stdlib.h>
#include <stdio.h>

#include "error.h"

void malloc_error(void) {
    perror("malloc");
    exit(EXIT_FAILURE);
}
void realloc_error(void) {
    perror("realloc");
    exit(EXIT_FAILURE);
}
void calloc_error(void) {
    perror("calloc");
    exit(EXIT_FAILURE);
}
