#define _GNU_SOURCE
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_N 100

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t work = 0, pause_work = 0, work2 = 0, end = 0, end2 = 0;
pid_t children[MAX_N];

void usr1_handler(int sig){
    work = 1;
    pause_work = 0;
}

void usr1_handler2(int sig){
    work2 = 1;
}

void usr2_handler(int sig){
    pause_work = 1;
    work = 0;
}

void int_handler(int sig){
    end = 1;
}

void int2_handler(int sig){
    end2 = 1;
}

void sethandler(void (*f)(int), int sigNo){
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if(-1 == sigaction(sigNo, &act, NULL)){
        ERR("sigaction");
    }
}

void child_work(){
    int count = 0;
    printf("My pid:%d\n", getpid());
    srand(getpid());
    sethandler(usr1_handler, SIGUSR1);
    sethandler(usr2_handler, SIGUSR2);
    sethandler(int2_handler, SIGINT);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
        while(!work){
            sigsuspend(&oldmask);
        }
        while(end2 != 1){
            if(!pause_work){
            sigprocmask(SIG_SETMASK, &oldmask, NULL);
            int r = 100 + rand() % (200 - 100 + 1);
            struct timespec t = {1, 0};
            nanosleep(&t, NULL);
            count++;
            printf("%d: %d\n", getpid(), count);
            sigprocmask(SIG_BLOCK, &mask, NULL);
            }
            else{
                sigsuspend(&oldmask);
            }
        }
        char file[MAX_N];
        sprintf(file, "%d.txt", getpid());
        FILE *f = fopen(file, "w+");
        fprintf(f, "%d", count);
        fclose(f);
}

void create_children(int n){
    pid_t s;
    for(int i=0;i<n;i++){
        if((s = fork()) < 0){
            ERR("fork");
        }
        if(s == 0){
            child_work();
            exit(EXIT_SUCCESS);
        }
        else{
            children[i] = s;
        }
    }
}

int main(int argc, char** argv){
    printf("PARENT PID: %d\n", getpid());
    if(argc != 2){
        perror("zle wejscie");
    }
    int n = atoi(argv[1]);
    create_children(n);
    int i = 0;
    sethandler(usr1_handler2, SIGUSR1);
    sethandler(int_handler, SIGINT);
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    kill(children[0], SIGUSR1);
    while(end != 1){
        while(!work2){
            sigsuspend(&oldmask);
        }
        work2 = 0;
        kill(children[i], SIGUSR2);
        i = (i+1) % n;
        kill(children[i], SIGUSR1);
    }
    sethandler(SIG_IGN, SIGINT);
    kill(0, SIGINT);
    while(wait(NULL) > 0){

    }
    return EXIT_SUCCESS;
}