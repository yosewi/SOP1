#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s 0<n\n", name);
    exit(EXIT_FAILURE);
}

void child_work(int i){
    srand(time(NULL) * getpid());
    int t = 5 + rand() % (10 - 5 + 1);
    sleep(t); //czeka 5-10 sekund
    printf("Process with PID %d terminated\n", getpid());
}

void create_children(int n){
    pid_t s;
    for(int i=0;i<n;i++){
        if((s=fork()) < 0){
            ERR("fork");
        }
        if(s==0){
            child_work(n);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv){
    if(argc<2){
        usage(argv[0]);
    }

    int child_count = atoi(argv[1]);

    if(child_count<=0){
        usage(argv[0]);
    }

    create_children(child_count);
    while(child_count>0){
        sleep(3);
        pid_t pid;
        while(1){
            pid = waitpid(0, NULL, WNOHANG);
            if(pid > 0){
                child_count--;
            }
            if(0 == pid){
                break;
            }
            if(0 >= pid){
                if(ECHILD == errno){
                    break;
                }
                ERR("waitpid");
            }
        }
        printf("Parent: %d processess remain\n", child_count);
    }
    return EXIT_SUCCESS;

}