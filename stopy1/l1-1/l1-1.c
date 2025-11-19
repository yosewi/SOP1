#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))


void scan_dir(){
    DIR *d;
    struct dirent *dp;
    struct stat file_stat;
    int files = 0, dirs = 0, links = 0, others = 0;
    if((d = opendir(".")) == NULL){
        ERR("opendir");
    }
    do{
        errno = 0;
        if((dp = readdir(d)) != NULL){
            if(lstat(dp->d_name, &file_stat)){
                ERR("lstat");
            }
            if(S_ISREG(file_stat.st_mode)){
                files++;
            }
            else if(S_ISDIR(file_stat.st_mode)){
                dirs++;
            }
            else if(S_ISLNK(file_stat.st_mode)){
                links++;
            }
            else{
                others++;
            }
        }
    }while(dp != NULL);
    if(errno != 0){
        ERR("errno");
    }
    if(closedir(d)){
        ERR("closedir");
    }
    printf("Files: %d\nDirs: %d\nLinks: %d\nOthers: %d\n", files, dirs, links, others);
}


int main(int argc, char **argv){
    scan_dir();
    return EXIT_SUCCESS;
}