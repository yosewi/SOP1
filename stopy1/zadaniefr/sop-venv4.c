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

void rp(const char *env, const char *pkg){
    char path[MAXPATH];
    if(getcwd(path, MAXPATH) == NULL){
        ERR("getcwd");
    }
    if(chdir(path)){
        ERR("chdir");
    }
    if(chdir(env)){
        fprintf(stderr, "sop-venv: the environment does not exist\n");
        ERR("chdir env");
    }
    FILE *f = fopen("requirements", "r");
    if(!f){
        ERR("fopen requirements");
    }

    char buf[1024][128]; // maks. 1024 pakiety, każdy do 128 znaków
    int count = 0;
    char line[128];
    int found = 0;

    while(fgets(line, sizeof(line), f)){ //czyta linie z pliku f
        char tmp[128]; //tmp
        strncpy(tmp, line, sizeof(tmp)); //kopiuje do tmp <- line
        tmp[sizeof(tmp)-1]='\0'; //ustawiam znak konca lancucha
        char *name = strtok(tmp, " \n"); // dzieli linie tak, ze bierzemy nazwe pakietu, bo to dzieli tmp w momencie jak bedzie spacja
        if(name && strcmp(name, pkg) != 0){ //jesli to nie pakiet ktory chcemy usunac to
            strncpy(buf[count], line, sizeof(buf[count])); //kopiujemy linie do buf
            buf[count][sizeof(buf[count])-1] = '\0';
            count++; //i zwiekszamy count
        } else if(name && strcmp(name, pkg) == 0){
            found = 1; // znaleziono pakiet do usunięcia
        }
    }
    fclose(f);
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
    char *envs[32];
    int env_count = 0;
    char *pkg = NULL;
    char *rm_pkg = NULL;
    while((c = getopt(argc, argv, "cv:i:r:")) != -1){
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
            case 'r':
                rm_pkg = optarg;
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
    if(rm_pkg != NULL && env_count>0){
        for(int j=0;j<env_count;j++){
            rp(envs[j], rm_pkg);
        }
    }
    return EXIT_SUCCESS;
}