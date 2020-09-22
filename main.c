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

void cat(char** cmd, int in_pos, int out_pos) {
    int in = open(cmd[in_pos], O_RDONLY);
    int out = open(cmd[out_pos], O_WRONLY | O_APPEND | O_CREAT, S_IRUSR
        | S_IRGRP | S_IWGRP | S_IWUSR); // O_APPEND

    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);

    close(in);
    close(out);
}

char** get_cmd() {
    char *buffer = malloc(BUFSIZE * sizeof(char));
    char **cmd = malloc(sysconf(_SC_ARG_MAX) * sizeof(char *));
    size_t nalloc_buf = 0, nalloc = 0, nused = 0, i = 0;
    int c, ikey = 0, okey = 0;

    if (cmd == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    printf("\nbilly@billy-shell> ");
    /*if (!fgets(buffer, BUFSIZE, stdin)) {
        perror("fgets");
        exit(EXIT_FAILURE);
    }*/

    /* Going through cmd to look for special characters >, <, >> */
    while(1) {
        c = getchar();
        if (c == EOF || c == '\n') {
            buffer[i] = '\0';
            printf("added null");
            break;
        }

        if (c == '>') {
            buffer[i] = ' ';
            okey++;
        } else if (c == '<') {
            buffer[i] = ' ';
            ikey++;
        } else {
            buffer[i] = c;
            printf("%ld", i);
        }
        i++;

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
    char *token = strtok(buffer, " \n\0");
    while (token != NULL) {
        cmd[nused] = token;
        if (ikey == 1 && (strstr(token, ".txt") != NULL))
            input = nused;
        if (okey == 1 && (strstr(token, ".txt") != NULL))
            output = nused;

        nused++;
        token = strtok(NULL, " \n\0");
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

int execute_cmd(char** cmd) {
    pid_t child_pid, wpid;
    int status;

    child_pid = fork(); // make a copy of the process, parent and child
    if (child_pid == -1) {      /* fork() failed */
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {       /* This is the child */
        /* Child does some work and then terminates */
        int ret;

        if (strcmp(cmd[0], "exit") == 0) {
            printf("Exited\n");
            exit(EXIT_SUCCESS);
        }
        if (strcmp(cmd[0], "cat") == 0 && output != 0)
            cat(cmd, input, output);

        ret = execvp(cmd[0], cmd);

        if (ret == -1) {
            perror("execve");
            exit(EXIT_FAILURE);
        }
        free(cmd);
        fprintf(stderr, "Unknown error code %d from execl\n", ret);
        exit(EXIT_FAILURE);
        //}
    } else {                    /* This is the parent */
        do {
            wpid = waitpid(child_pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return EXIT_FAILURE;
}

void run() {
    while(1) {
        int exec = execute_cmd(get_cmd());
        if (!exec) {
            perror("cannot execute commands");
            break;
        }
    }
}


int main() {
    run();
    return 0;
}