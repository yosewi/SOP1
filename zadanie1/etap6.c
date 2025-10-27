#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <bits/getopt_core.h> // niepotrzebne ale IDE nie widziało optarg
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#define MAXPATH 101
#define FILE_BUF_LEN 256

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(const char *const pname)
{
    fprintf(stderr, "USAGE:%s -p directory -o file_name\n", pname);
    exit(EXIT_FAILURE);
}

void scan_dir(int f){
    errno = 0;
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
            dprintf(f, "%s %ld\n", dp->d_name, filestat.st_size);
        }
    }while(dp!=NULL);
    if(closedir(d)){
        ERR("closedir");
    }
    printf("%d", errno);
}

int main(int argc, char **argv){
    int c;
    int p =0;
    int i=0;
    char *pom;
    int f = 1;
    char cwd[MAXPATH];
    if(getcwd(cwd,MAXPATH) == NULL){
        ERR("cwd");
    }

    while((c = getopt(argc, argv, "p:o:")) != -1){
        switch (c){
            case 'o':
                if(p > 0){
                    usage(argv[0]);
                }
                const char *const path = optarg;
                if (unlink(path) && errno != ENOENT)
                    ERR("unlink");
                f = open(path, O_WRONLY | O_CREAT, 0777);
                if(f == -1){
                    ERR("open");
                }
                i=1;
                p++;
                break;
            case 'p':
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }

    optind = 1;

    while((c = getopt(argc,argv, "p:o:")) != -1){
        switch(c){
            case 'p':
                errno = 0;
                if(chdir(optarg)){
                    ERR("chdir");
                }
                pom = optarg;
                break;
            case 'o':
                break;
            case '?':
            default:
                usage(argv[0]);
        }
        write(f, "SCIEZKA:\n", 10);
        printf("%d", errno);
        dprintf(f, "%s\nLISTA PLIKOW:\n", pom);
        scan_dir(f);
        if(chdir(cwd)){
            ERR("chdir");
        }
    }
    if(i==1){
        if(f != 1 && close(f)){
            ERR("fclose");
        }
    }
    return EXIT_SUCCESS;
}   