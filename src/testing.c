#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

char *proc = "/proc";

int print_dir(const char *path, char *arr[]) {
    // refs: https://faq.cprogramming.com/cgi-bin/smartfaq.cgi?answer=1046380353&id=1044780608
    //      https://stackoverflow.com/questions/7631054/how-to-return-an-array-of-strings-in-c
/*
 * TODO
 *   * malloc arr: get amount of files in dir first then use that
 */
    struct dirent *dir;
    DIR *d = opendir(path);
    int count = 0;
    if (d)
    {
        while (dir = readdir(d))
        {
            size_t len = strlen(dir->d_name);
            arr[count] = malloc(len + 1);
            if(!arr[count]) {
                free(arr[count]);
                return -1;
            }
            strncpy(arr[count], dir->d_name, len);
//            printf("%s\nstrs[%d] = %s\n", dir->d_name,count, arr[count]);
            count++;
        }

        closedir(d);
    }
    return 0;
}

int dir_size(const char *path) {

    DIR           *d;
    struct dirent *dir;
    d = opendir(path);
    int count = 0;
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            count++;
        }

        closedir(d);
    }
    return count;
}


const char *add_proc(const char *path) {

    size_t len = strlen(proc) + strlen(path);
    char *final_path = (char *) malloc(len);
//    strcpy(final_path, proc);
    snprintf(final_path, len + 1, "%s%s", proc, path);
//    strcat(final_path, path);
    return final_path;
}

int main(int *argc, char *argv[]) {
/*
 * TODO
 *   * get print_dir to work in here
 */

    const char *pc_file = "/proc/net/dev";
    const char *pc_dir = "/net";

    const char *final = add_proc(pc_dir);
    printf("%s%s", final, final);



//    int pc_size = dir_size(pc_dir);
//    char *tmp[pc_size];
//    print_dir(pc_dir, tmp);
//    printf("size: %d", pc_size);
//
//    for(int i = 0; i < pc_size; i++) {
//            printf("strs[%d] = %s\n", i, tmp[i]);
//    }

    return 0;
}
