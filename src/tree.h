#ifndef PROCSYS_TREE_H
#define PROCSYS_TREE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXPATH 64

typedef struct node {
    struct node *parent, *child, *sibling;
    char type; // F or D
    char name[MAXPATH];
} NODE;

/* Create a new node. Will update it's parent/child/sibling nodes
 * if !NULL
 * @input
 * df: char either 'D' for directory or 'F' for file
 * name: the directory/file name for this NODE
 * pnt: the parent NODE, 0 or NULL if unknown
 * cld: the child NODE, 0 or NULL if unknown
 * sib: the sibling NODE, 0 or NULL if unknown
 * @return
 * NODE type
 */
NODE *create_node(char df, char name[], NODE *pnt, NODE *cld, NODE *sib);

void print_cwd();
NODE *visit_sibling();
NODE *visit_child();
NODE *visit_parent();
NODE *visit_root();

#endif //PROCSYS_TREE_H
