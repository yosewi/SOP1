#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
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
    if((f = fopen("requirements", "a+")) == NULL){
        ERR("fopen");
    }
    if(fclose(f)){
        ERR("fclose");
    }
    if(chdir(path)){
        ERR("chdir");
    }
}

void ip(const char *env, const char *pkg){
    char path[MAXPATH];
    if(getcwd(path,MAXPATH) == NULL){
        ERR("getcwd");
    }
    if(chdir(path)){
        ERR("chdir");
    }
    if(chdir(env)){
        ERR("chdir");
    }
    FILE *f;
    if((f = fopen("requirements", "a+")) == NULL){
        ERR("fopen");
    }
    char *version;
    char *package;
    char buf[128];
    strncpy(buf, pkg, sizeof(buf));
    char *sep = strstr(buf, "==");
    if(sep == NULL){
        fprintf(stderr, "Invalid package\n");
    }
    *sep = '\0';
    package = buf;
    version = sep + 2;
    fprintf(f, "%s %s\n", package, version);
    if(fclose(f)){
        ERR("fclose");
    }
    FILE *fl;
    if((fl = fopen(package, "w")) == NULL){
        ERR("fopen");
    }
    int len = 20 + rand() % 30;
    for(int i=0;i<len;i++){
        char c = 'a' + rand() % 50;
        fputc(c, fl);
    }
    fputc('\n', fl);
    if(fclose(fl)){
        ERR("fclose");
    }
    if(chmod(package, 0444) == -1){
        ERR("chmod");
    }
    if(chdir(path)){
        ERR("chdir");
    }
}

int main(int argc, char **argv){
    int c;
    int i=0;
    char *envs[32];
    int env_count = 0;
    char *pkg = NULL;
    while((c = getopt(argc, argv, "cv:i:")) != -1){
        switch(c){
            case 'c':
                i=1;
                break;
            case 'v':
                if(env_count < 32){
                    envs[env_count++] = optarg;
                }
                break;
            case 'i':
                pkg = optarg;
                break;
        }
    }
    if(i == 1 && env_count>0){
        for(int j=0;j<env_count;j++){
            ce(envs[j]);
        }
    }
    if(pkg != NULL && env_count>0){
        for(int j=0;j<env_count;j++){
            srand(time(NULL));
            ip(envs[j], pkg);
        }
    }
    return EXIT_SUCCESS;
}