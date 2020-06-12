#ifndef PROCSYS_TREE_H
#define PROCSYS_TREE_H

#define MAXPATH 64

extern struct node *root;

typedef struct node {
    struct node *parent, *child, *sibling;
    char type; // F or D
    char name[MAXPATH];
} NODE;

#endif //PROCSYS_TREE_H
