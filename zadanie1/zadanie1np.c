#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH_LENGTH 100

void write_dir_content(const char* dir_path, char* out_path) {
    DIR* current_dir = opendir(dir_path);
    if (current_dir == NULL) ERR("opendir()");

    FILE* output;
    if (out_path == NULL) output = stdout;
    else {
        output = fopen(out_path, "a");
    }

    char old_path[MAX_PATH_LENGTH];
    if (!getcwd(old_path, MAX_PATH_LENGTH)) ERR("getcwd()");
    if (chdir(dir_path)) ERR("chdir()");

    fprintf(output, "SCIEZKA:\n");
    fprintf(output, "%s\n", dir_path);
    fprintf(output, "LISTA PLIKOW:\n");

    struct dirent* current_file;
    struct stat file_info;
    while ((current_file = readdir(current_dir)) != NULL) {
        if (lstat(current_file->d_name, &file_info)) ERR("lstat");
        fprintf(output, "%s %ld\n", current_file->d_name, file_info.st_size);
    }

    if (chdir(old_path)) ERR("chdir()");
    closedir(current_dir);
}

void write_dir_content2(const char* dir_path, char* out_path) {
    errno = 0;
    DIR* current_dir = opendir(dir_path);
    if (current_dir == NULL) ERR("opendir()");

    int output;
    if (out_path == NULL) output = 1;
    else {
        output = open(out_path, O_APPEND|O_CREAT|O_WRONLY, 0777);
    }

    char old_path[MAX_PATH_LENGTH];
    if (!getcwd(old_path, MAX_PATH_LENGTH)) ERR("getcwd()");
    if (chdir(dir_path)) ERR("chdir()");
    printf("%d", output);
    errno = 0;

    write(output, "SCIEZKA:\n", 10);
    printf("%s", strerror(errno));
    dprintf(output, "%s\n", dir_path);
    dprintf(output, "LISTA PLIKOW:\n");

    struct dirent* current_file;
    struct stat file_info;
    while ((current_file = readdir(current_dir)) != NULL) {
        if (lstat(current_file->d_name, &file_info)) ERR("lstat");
        dprintf(output, "%s %ld\n", current_file->d_name, file_info.st_size);
    }

    if (chdir(old_path)) ERR("chdir()");
    if (output != 1) close(output);
    close(output);
    closedir(current_dir);
    printf("%d", errno);
}

int main(int argc, char** argv) {
    int c;
    int mark = 0;
    char output_path[MAX_PATH_LENGTH];
    while ((c = getopt(argc, argv, "p:o:")) != -1) {
        switch (c) {
            case 'p':
                break;
            case 'o':
                strcpy(output_path, optarg);
                mark = 1;
                break;
            case '?':
            default:
                ERR("arguments");
        }
    }
    if (unlink(output_path) && errno != ENOENT) ERR("unlink");
    optind = 1;
    while ((c = getopt(argc, argv, "p:o:")) != -1) {
        switch (c) {
            case 'p':
                if (mark == 0) write_dir_content2(optarg, NULL);
                else write_dir_content2(optarg, output_path);
                break;
            case 'o':
                break;
            case '?':
            default:
                ERR("arguments");
        }
    }

    return EXIT_SUCCESS;
}