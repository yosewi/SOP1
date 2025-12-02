#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source) \
    (kill(0, SIGKILL), perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define FILE_MAX_SIZE 512

volatile sig_atomic_t usr1_count = 0;
volatile sig_atomic_t usr2_count = 0;
volatile sig_atomic_t int_count = 0;

void usr1_handler(int sigNo){
    usr1_count = 1;
}

void usr2_handler(int sigNo){
    usr2_count = 1;
}

void int_handler(int sigNo){
    int_count = 1;
}

void usage(int argc, char* argv[])
{
    printf("%s p h\n", argv[0]);
    printf("\tp - path to directory describing the structure of the Austro-Hungarian office in Prague.\n");
    printf("\th - Name of the highest administrator.\n");
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo){
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if(sigaction(sigNo, &act, NULL) == -1){
        ERR("sigaction");
    }
}

ssize_t bulk_read(int fd, char *buf, size_t count){
    ssize_t c;
    ssize_t len = 0;
    do{
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if(c < 0){
            return c;
        }
        if(c == 0){
            return len;
        }
        buf += c;
        len += c;
        count -= c;
    } while(count > 0);
    return len;
}

void etap2(char* name, sigset_t* oldmask, int is_root){
    pid_t s;
    pid_t pids[2];
    int childs_count = 0;
    printf("My name is %s, and my pid is %d\n", name, getpid());

    int f = open(name, O_RDONLY);
    if(f == -1){
        ERR("open");
    }

    char file_buf[FILE_MAX_SIZE];
    ssize_t size = bulk_read(f, file_buf, FILE_MAX_SIZE);
    if(size == -1){
        ERR("bulk_read");
    }

    close(f);

    file_buf[size] = '\0';

    char* child_name = strtok(file_buf, "\n");

    while(child_name != NULL){
        if(strcmp(child_name, "-") == 0){
            child_name = strtok(NULL, "\n");
            continue;
        }
        printf("%s inspecting %s\n", name, child_name);

        if((s = fork()) < 0){
            ERR("fork");
        }
        if(s == 0){
            etap2(child_name, oldmask, 0);
            exit(EXIT_SUCCESS);
        }

        pids[childs_count++] = s;

        child_name = strtok(NULL, "\n");
    }

    printf("%s has inspected all subordinates\n", name);

    srand(getpid());
    while(sigsuspend(oldmask)){
        if(int_count){
            printf("%s ending the day\n", name);
            if(pids[0] > 0){
                kill(pids[0], SIGINT);
            }
            if(pids[1] > 0){
                kill(pids[1], SIGINT);
            }
            break;
        }
        if(usr2_count){
            usr2_count = 0;
            switch(rand() % 3){
                case 0:
                case 1:
                    if(is_root){
                        printf("%s received a document. Ignoring.\n", name);
                    }
                    else{
                        printf("%s received a document. Passing on to superintendent\n", name);
                        kill(getppid(), SIGUSR2);
                    }
                    break;
                case 2:
                    printf("%s received a document. Sending to the archive.\n", name);
                    break;
            }
        }
    }

    for(int i = 0; i<childs_count; i++){
        waitpid(pids[i], NULL, 0);
    }

    printf("%s leaving the office\n", name);
}

int main(int argc, char** argv)
{
    pid_t pid = getpid(); 
    printf("My pid: %d\n", pid);
    if(argc != 3){
        usage(argc, argv);
    }

    if(chdir(argv[1])){
        ERR("chdir");
    }

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    sethandler(usr1_handler, SIGUSR1);
    sethandler(usr2_handler, SIGUSR2);
    sethandler(int_handler, SIGINT);

    printf("Waiting for SIGUSR1\n");

    while(usr1_count != 1 && sigsuspend(&oldmask)) {}

    printf("USR1 received\n");

    etap2(argv[2], &oldmask, 1);
}