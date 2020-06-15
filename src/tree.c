#include "tree.h"
/*                      DIR/file
              / - root
              \ - sibling
             A(/A)
    child - / \
     (/A/C)C   B(/B)
          /     \
  (/A/C/d)d      e(/e)

  TODO
    * Root doesn't have parent or sibling
    * Must order the siblings alphabetically
    * Search: follow_sibling to check siblings/stay same level
              follow_child to enter into that dir
 */

NODE *create_node(char df, char name[], NODE *pnt, NODE *cld, NODE *sib) {
/* TODO
 *   * Need a MAXNAME var to control name before giving to this function
 */
    NODE *new_node = (NODE *)malloc(sizeof(NODE));
    strncpy(new_node->name, name, strlen(name));
    new_node->type = df;

    if(pnt) {
        pnt->child = new_node;
    }
    if(cld) {
        cld->parent = new_node;
    }
    if(sib) {
        sib->sibling = new_node;
    }
    new_node->parent = pnt;
    new_node->child = cld;
    new_node->sibling = sib;

    return new_node;
}

int main(int argc, char *argv[]) {
    NODE *root = create_node('D', "/", 0, 0, 0);
    NODE *dirA = create_node('D', "A", root, 0, 0);

    printf("dirA's parent is: %s\n", dirA->parent->name);
    printf("root's child is: %s\n", root->child->name);
    return 0;
}
