#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <bits/getopt_core.h> // niepotrzebne ale IDE nie widziało optarg
#define MAXPATH 256

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void ce(const char *name){
    struct stat st;
    if(lstat(name, &st) == 0){
        fprintf(stderr, "environment already exists\n");
        ERR("lstat");
    }
    if(mkdir(name, 0777) == -1){
        ERR("mkdir");
    }
    char path[MAXPATH];
    if(getcwd(path, MAXPATH) == NULL){
        ERR("getcwd");
    }
    if(chdir(name)){
        ERR("chdir");
    }
    FILE *f;
    if((f = fopen("requirements", "w+")) == NULL){
        ERR("fopen");
    }
    if(fclose(f)){
        ERR("fclose");
    }
    if(chdir(path)){
        ERR("chdir");
    }
}

int main(int argc, char **argv){
    int c;
    int i=0;
    char *env;
    while((c = getopt(argc, argv, "cv:")) != -1){
        switch(c){
            case 'c':
                i=1;
                break;
            case 'v':
                env = optarg;
                break;
        }
    }
    if(i == 1 && env){
        ce(env);
    }
    return EXIT_SUCCESS;
}