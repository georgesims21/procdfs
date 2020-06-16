#include "tree.h"
/*                      DIR/file
              / - root
              /
             A(/A)
    child - / \ - sibling
     (/A/C)C   B(/B)
          /     \
  (/A/C/d)d      e(/e)

  TODO
    * Root doesn't have parent or sibling
    * Must order the siblings alphabetically
    * Search: follow_sibling to check siblings/stay same level
              follow_child to enter into that dir
 */

NODE *tree_root;
NODE *cwd;

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

int init_tree() {

    tree_root = create_node('D', "/", 0, 0, 0);
    if(!tree_root) {
        return -1;
    }
    cwd = tree_root;
    return 0;
}

void print_cwd() {printf("cwd: | %s |\n", cwd->name);}
NODE *visit_sibling() {cwd = (cwd->sibling) ? cwd->sibling : NULL;}
NODE *visit_child() {cwd = (cwd->child) ? cwd->child : NULL;}
NODE *visit_parent() {cwd = (cwd->parent) ? cwd->parent : NULL;}
NODE *visit_root() {cwd = (tree_root) ? tree_root : NULL;}

//int main(int argc, char *argv[]) {
//    init_tree();
//    print_cwd();
//    NODE *dirA = create_node('D', "A", tree_root, 0, 0);
//    NODE *dirB = create_node('D', "B", tree_root, 0, dirA);
//    visit_child();
//    print_cwd();
//    visit_sibling();
//    print_cwd();
//    visit_sibling();
//    print_cwd();
//    visit_parent();
//    print_cwd();
//    visit_child();
//    print_cwd();
//    visit_root();
//    print_cwd();
//
//    return 0;
//}

