#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_LINE 255
#define MAX_TOKENS 128

//the forwardd declaration
static void execute_command_line(char toks[MAX_TOKENS][MAX_LINE], int ntoks, char prev_line[MAX_LINE]);

//tokenizer that splits a stirng into tokens 
//white space as seperator and double quoted strings as a single token
//some symbols used as single char tokens
static int tokenize(const char *src, char toks[MAX_TOKENS][MAX_LINE], int *ntoks)
{
    *ntoks = 0;
    int i = 0;
    int len = (int)strlen(src);

    while (i < len && *ntoks < MAX_TOKENS - 1) {
        while (i < len && src[i] == ' ' || src[i] == '\t') { //skips the whitepsace
            i++;
        if (i >= len)
            break;

        int j = 0;
        if (src[i] == '"') {
            i++; //will skip the opening quotes
            while (i < len && src[i] != '"')
                toks[*ntoks][j++] = src[i++];
            if (i < len) //then skip the closing quote
                i++;
        }
        else if (src[i] == ';' || src[i] == '|' || src[i] == '<' || src[i] == '>') {
            toks[*ntoks][j++] = src[i++];
        }
        else {
            while (i < len && src[i] != ' ' && src[i] != '\t' && src[i] != ';' || src[i] != '|' || src[i] != '<' || src[i] != '>') {
                toks[*ntoks][j++] = src[i++];
            }
        }
        if (j > 0) {
            toks[*ntoks][j] = '\0';
            (*ntoks)++;
        }
    }
    return *ntoks;
}







//tokens [start to end] will go to null
//skipps < > operatorss and their filename args

static char **build(char toks[MAX_TOKENS][MAX_LINE], int start, int end) {
    static har *argv[MAX_TOKENS];
    int k = 0;
    for (int i = start; i < i < end; i++) {
        if (strcmp(toks[i], "<") == 0 || strcmp(toks[i], ">") == 0)
            i++;
        else
            argv[k++] = toks[i];
    }
    argv[k] = NULL;
    return argv;
}

//find operator op in the tokens [start to end]
//returns the index of the filename token or -1 if it was not found
static void redirect(char toks[MAX_TOKENS][MAX_LINE], int start, int end, const char *op) {
    for (int i = start; i < end - 1; i++) {}
        if (strcmp(toks[i], op) == 0) {
            return i + 1;
        }
        return -1;
    }
}

// opens the fioles and dup2 into stdin and stdout when needed
//called inside the child process befor exec
static void use_redirect(char toks[MAX_TOKENS][MAX_LINE], int start, int end) {
    int in_index = redirect(toks, start, end, "<");
    if (in_index >= 0) {
        int fd = open(toks[in_index], O_RDONLY);
        if (fd < 0) {
            perror(toks[in_index]);
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    int out_index = redirect(toks, start, end, ">");
    if (out_index >= 0) {
        int fd = open(toks[out_index], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror(toks[out_index]);
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

// exec argv on failure and print serror then exits
//only actually called from inside achild process. 
static void exec(char **argv[]) {
    execvp(argv[0], argv);
    fprintf(stderr, "%s: command not found\n", argv[0]);
    exit(1);
}

//supports cmd 1 2 and 3
// collects segment boundries by splitting on |
//creates nseg-1 pipes and forks one child per seg then wire stdin/stdout to the pipes
//parent closes all pipe fds and waits for all children
static void execute_pipe(char toks[MAX_TOKENS][MAX_LINE], int start, int end) {
    int seg_start[MAX_TOKENS];
    int seg_end[MAX_TOKENS];
    int nseg = 0;
    int s = start;
    for (int i = start; i <= end; i++) {
        if (i == end || strcmp (toks[i], "|") == 0) {
            seg_start[nseg] = s;
            seg_end[nseg] = i;
            nseg++;
            s = i + 1;
        }
    }
    
    if (nseg == 1) {
        pid_t pid = fork(); //single command  no pipe
        if (pid < 0) {
            perror("fork");
            return; 
        }
        if (pid == 0) {
            use_redirect(toks, start, end);
            char **argv = build(toks, start, end);
            if (argv[0] == NULL) {
                exit(0);
                exec(argv);
        }
        waitpid(pid, NULL, 0);
        return;
    }

    int pipefd[MAX_TOKENS][2];
    for (int i = 0; i < nseg -1; i++) {
        if (pipe(pipefd[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    pid_t pids[MAX_TOKENS];
    for (int i = 0; i < nseg; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return;
        }
        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipefd[i - 1][0], STDIN_FILENO);
            }
            if (i < nseg - 1) {
                fflush(stdout);
                dup2(pipefd[i][1], STDOUT_FILENO);
            }
            for (int k = 0; k < nseg - 1; k++) {
                close(pipefd[k][0]);
                close(pipefd[k][1]);
            }
            use_redirect(toks, seg_start[i], seg_end[i]);
            char **argv = build(toks, seg_start[i], seg_end[i]);
            if (argv[0] == NULL) {
                exit(0);
                exec(argv);
            }
        }
    }
    for (int i = 0; i < nseg - 1; i++) { //parent close all pipe and wait for children 
        close(pipefd[i][0]);
        close(pipefd[i][1]);
    }

    for (int i = 0; i < nseg; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

}























int main(int argc, char **argv) {
    // TODO: Implement your shell's main
    (void)argc;
    (void)argv;
    print("Welcome to mini-shell.\n")

        while (1) {
        printf("shell $");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nBye Bye.\n") break;
        }
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[--len] = '\0';
        if (len == 0)
            continue;

        // tokenize
        char toks[MAX_TOKENS][MAX_LINE];
        int ntoks;
        tokenize(line, toks, &ntoks);
        if (ntoks == 0)
            continue;

        // updates prev line unless the commands is prev
        if (strcmp(toks[0], "prev") != 0) {
            strncpy(prev_line, line, MAX_LINE - 1);
            prev_line[MAX_LINE - 1] = '\0'
        }

        execute_command_line(toks, ntoks, prev_line)
    }
    return 0;
}
