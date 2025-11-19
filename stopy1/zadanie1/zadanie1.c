#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <bits/getopt_core.h> // niepotrzebne ale IDE nie widziało optarg
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#define MAXPATH 101

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(const char *const pname)
{
    fprintf(stderr, "USAGE:%s -p path -p path ... -o output\n", pname);
    exit(EXIT_FAILURE);
}

void scan_dir(char *dirname, char *filename, int mark){
    DIR *d;
    struct dirent *dp;
    struct stat filestat;
    int f = 1;
    char path[MAXPATH];
    if(getcwd(path,MAXPATH) == NULL){
        ERR("getcwd");
    }
    if(mark == 1){
        if((f = open(filename, O_APPEND|O_WRONLY|O_CREAT, 0777)) == -1){
            ERR("fopen");
        }
    }
    if(chdir(dirname)){
        fprintf(stderr, "%s is a non existing path, albo nie masz dostepu\n", dirname);
        chdir(path);
        return;
    }
    if((d = opendir(".")) == NULL){
        ERR("opendir");
    }
    write(f, "SCIEZKA:\n", 10);
    dprintf(f, "%s\nLISTA PLIKOW:\n", dirname);
    do{
        if((dp = readdir(d)) != NULL){
            if(lstat(dp->d_name, &filestat)){
                ERR("lstat");
            }
            dprintf(f, "%s %ld\n", dp->d_name, filestat.st_size);
        }
    }while(dp!=NULL);
    if(mark == 1){
        if(close(f)){
        ERR("fclose");
    }
    }
    if(closedir(d)){
        ERR("closedir");
    }
    if(chdir(path)){
        ERR("chdir");
    }
}

int main(int argc, char **argv){
    int c;
    char *filename = NULL;
    int mark=0;
    while((c = getopt(argc, argv, "p:o:")) != -1){
        switch(c){
            case 'p':
                break;
            case 'o':
                filename = optarg;
                mark+=1;
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }

    if(mark > 1){
        usage(argv[0]);
        ERR("too many -o arguments\n");
    }

    optind = 1;

    while((c = getopt(argc, argv, "p:o:")) != -1){
        switch(c){
            case 'p':
                scan_dir(optarg, filename, mark);
                break;
            case 'o':
                break;
            case'?':
            default:
                usage(argv[0]);
        }
    }
    return EXIT_SUCCESS;
}