#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <bits/getopt_core.h> // niepotrzebne ale IDE nie widziało optarg
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#define MAXPATH 101

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void scan_dir(FILE *f){
    DIR* d;
    struct dirent *dp;
    struct stat filestat;
    if((d = opendir(".")) == NULL){
        ERR("opendir");
    }
    do{
        if((dp = readdir(d))!=NULL){
            if(lstat(dp->d_name, &filestat)){
                ERR("lstat");
            }
            fprintf(f, "%s %ld\n", dp->d_name, filestat.st_size);
        }
    }while(dp!=NULL);
    if(closedir(d)){
        ERR("closedir");
    }
}

int main(int argc, char **argv){
    int c;
    int i=0;
    char *pom;
    FILE *f = stdout;
    char cwd[MAXPATH];
    if(getcwd(cwd,MAXPATH) == NULL){
        ERR("cwd");
    }

    while((c = getopt(argc, argv, "p:o:")) != -1){
        switch (c){
            case 'o':
                umask(~0777);
                if (unlink(optarg) && errno != ENOENT)
                    ERR("unlink");
                if((f = fopen(optarg, "w+")) == NULL){
                    ERR("fopen");
                } 
                i=1;
                break;
        }
    }

    optind = 1;

    while((c = getopt(argc,argv, "p:o:")) != -1){
        switch(c){
            case 'p':
                if(chdir(optarg)){
                    ERR("chdir");
                }
                pom = optarg;
                break;
        }
        fprintf(f, "SCIEZKA:\n%s\nLISTA PLIKOW:\n", pom);
        scan_dir(f);
        if(chdir(cwd)){
            ERR("chdir");
        }
    }
    if(i==1){
        if(f != stdout && fclose(f)){
            ERR("fclose");
        }
    }
    return EXIT_SUCCESS;
}   