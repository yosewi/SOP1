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
#define MAXENV 20
#define MAXPKG 20

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void create_env(const char *name){
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

void install_package(char *env, char *pkg){
    char path[MAXPATH];
    if(getcwd(path, MAXPATH) == NULL){
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
    version = sep + 2;
    package = buf;
    fprintf(f, "%s %s\n", package, version);
    if(fclose(f)){
        ERR("fclose");
    }
    FILE *fl;
    if((fl = fopen(package, "a+")) == NULL){
        ERR("fopen");
    }
    int len = 20 + rand() % 30;
    for(int i=0;i<len;i++){
        char c = 'a' + rand() % 30;
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

void remove_package(char *env, char *pkg){
    char path[MAXPATH];
    if(getcwd(path, MAXPATH) == NULL){
        ERR("getcwd");
    }
    if(chdir(path)){
        ERR("chdir");
    }
    if(chdir(env)){
        ERR("chdir");
    }
    FILE *f;
    if((f = fopen("requirements", "r")) == NULL){
        ERR("fopen");
    }
    char buf[MAXPKG][128];
    int count = 0;
    char line[128];
    int found = 0;
    while(fgets(line, sizeof(line), f)){
        char tmp[128];
        strncpy(tmp, line, sizeof(line));
        tmp[sizeof(tmp) - 1] = '\0';
        char *name = strtok(tmp, " \n");
        if(name && strcmp(name, pkg) != 0){
            strncpy(buf[count], line, sizeof(buf[count]));
            buf[count][sizeof(buf[count])-1] = '\0';
            count++;
        }
        else if(name && strcmp(name, pkg) == 0){
            found = 1;
        }
    }
    if(fclose(f)){
        ERR("fclose");
    }
    if(!found){
        fprintf(stderr, "sop-venv: package '%s' not installed\n", pkg);
        exit(EXIT_FAILURE);
    }
    f = fopen("requirements", "w"); //to od razu czysci plik
    if(!f) ERR("fopen requirements write");
    for(int i=0;i<count;i++){
        fprintf(f, "%s\n", buf[i]);
    }
    fclose(f);
    if(unlink(pkg) == -1){
        ERR("unlink package");
    }
    if(chdir(path)){
        ERR("chdir back");
    }
}

int main(int argc, char **argv){
    int c;
    int i=0;
    char *env[MAXENV];
    int count_env = 0;
    char *pkg=NULL;
    char *rmpkg = NULL;
    while((c = getopt(argc, argv, "cv:i:r:")) != -1){
        switch(c){
            case 'c':
                i = 1;
                break;
            case 'v':
                env[count_env++] = optarg;
                break;
            case 'i':
                pkg = optarg;
                break;
            case 'r':
                rmpkg = optarg;
                break;
        }
    }
    if(i==1 && count_env>0){
        for(int j=0;j<count_env;j++){
            create_env(env[j]);
        }
    }
    if(pkg != NULL && count_env>0){
        for(int j=0;j<count_env;j++){
            srand(time(NULL));
            install_package(env[j], pkg);
        }
    }
    if(rmpkg != NULL && count_env>0){
        for(int j=0;j<count_env;j++){
            remove_package(env[j], rmpkg);
        }
    }
    return EXIT_SUCCESS;
}