#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define BLOCK 5
#define BUFSIZE 1024

int input = 0;
int output = 0;
int append = 0;

void cat(char** cmd, int in_pos, int out_pos, int append) {
    int in = open(cmd[in_pos], O_RDONLY);
    int out = open(cmd[out_pos], O_WRONLY | (append == 1 ? O_APPEND : O_TRUNC) | O_CREAT, S_IRUSR
        | S_IRGRP | S_IWGRP | S_IWUSR); // O_APPEND

    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);

    close(in);
    close(out);

    /*int ret;
    ret = execvp(cmd[0], cmd);
    fprintf(stderr, "Unknown error code %d from execl\n", ret);
    return EXIT_FAILURE;*/
}


char** get_cmd() {
    char *buffer = malloc(BUFSIZE * sizeof(char));
    char **cmd = malloc(sysconf(_SC_ARG_MAX) * sizeof(char *));
    size_t nalloc_buf = 0, nalloc = 0, nused = 0, i = 0;
    int c, ikey = 0, okey = 0;
    char prev_c;

    if (cmd == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    printf("billy@billy-shell> ");

    /* Going through cmd to look for special characters >, <, >> */
    while(1) {
        c = getchar();
        if (c == '>') {
            append = 0;
            buffer[i] = ' ';
            okey++;
            if (prev_c == c) {
                append++;
            }
            prev_c = c;
        } else if (c == '<') {
            buffer[i] = ' ';
            ikey++;
        } else {
            buffer[i] = c;
        }
        i++;
        if (c == EOF || c == '\n') {
            buffer[i] = '\0';
            break;
        }

        /* realloc */
        if (i == nalloc_buf) {
            char *tmp = realloc(buffer, ((nalloc_buf + BLOCK) * sizeof(char)));
            if (tmp == 0) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            buffer = tmp;
            nalloc_buf += BLOCK;
        }
    }

    /* Getting each command using strtok */
    char *token = strtok(buffer, " \n");
    while (token != NULL) {
        cmd[nused] = token;
        if (ikey == 1 && (strstr(token, ".txt") != NULL)) {
            input = nused;
        }
        if (okey == 1 && (strstr(token, ".txt") != NULL)) {
            output = nused;
        }
        nused++;
        token = strtok(NULL, " \n");
    }

    /* realloc */
    if (nused == nalloc) {
        char **tmp = realloc(cmd, ((nalloc + BLOCK) * sizeof(char *)));
        if (tmp == 0) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        cmd = tmp;
        nalloc += BLOCK;
    }
    return cmd;
}

void run(char** cmd) {
    if (strcmp(cmd[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }
}

int execute_cmd(char** cmd) {
    /* Get commands then fork() */
    pid_t child_pid;
    int status;
    run(cmd);
    child_pid = fork(); // make a copy of the process, parent and child
    if (child_pid == -1) {      /* fork() failed */
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (child_pid == 0) {       /* This is the child */
        /* Child does some work and then terminates */
        if (strcmp(cmd[0], "cat") == 0) {
            cat(cmd, input, output, append);
        }

        int ret;
        ret = execvp(cmd[0], cmd);

        if (ret == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "Unknown error code %d from execl\n", ret);
        return EXIT_FAILURE;
    } else {                    /* This is the parent */
        do {
            waitpid(child_pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return EXIT_FAILURE;
}


int main() {
    while(1) {
        char **arg = get_cmd();
        int exec = execute_cmd(arg);
        if (!exec) {
            perror("cannot execute commands");
            break;
        }
        free(arg);
    }
    return 0;
}