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
        while (i < len && (src[i] == ' ' || src[i] == '\t')) { //skips the whitepsace
            i++;
        }
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
            while (i < len && src[i] != ' ' && src[i] != '\t' && src[i] != ';' && src[i] != '|' && src[i] != '<' && src[i] != '>') {
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

//built in commands section

//change the working directory
static void builtin_cd(char toks[MAX_TOKENS][MAX_LINE], int ntoks) { 
    if (ntoks < 2) {
        fprintf(stderr, "cd missing arg\n");
        return;
    }
    if (chdir(toks[1]) != 0) {
        perror(toks[1]);
    }
}

//help with a list of built in commands
static void builtin_help(void) {
    printf("Built in commands are:\n");
    printf(" exit             - Exit the shell\n");
    printf(" cd <path>        - Change current working directory\n");
    printf(" source <file>    - Execute commands from a script file\n");
    printf(" prev             - print and re execute the previous commands\n");
    printf(" help.            - display this help message \n");
}

//source to exec each line of a file as a actual command 
static void builtin_source(char toks[MAX_TOKENS][MAX_LINE], int ntoks, char prev_line[MAX_LINE]) {
    if (ntoks < 2) {
        fprintf(stderr, "source is missing filename\n");
        return;
    }
    FILE *f = fopen(toks[1], "r");
    if (!f) {
        perror(toks[1]);
        return;
    }
    char line[MAX_LINE + 2];
    while (fgets(line, sizeof(line), f)) {
        size_t l = strlen(line);
        if (l > 0 && line[l - 1] == '\n') {
            line[--l] = '\0';
        }
        if (l == 0) {
            continue;
        }
        char subtoks[MAX_TOKENS][MAX_LINE];
        int nstoks;
        tokenize(line, subtoks, &nstoks);
        if (nstoks > 0) {
            execute_command_line(subtoks, nstoks, prev_line);
        }
    }
    fclose(f);
}

//returns 1 if the name is a shell built in 
static int is_builtin(const char *name) {
    return (strcmp(name, "exit") == 0 || 
            strcmp(name, "cd") == 0 ||
            strcmp(name, "source") == 0 ||
            strcmp(name, "prev") == 0 ||
            strcmp(name, "help") == 0);
}


//tokens [start to end] will go to null
//skipps < > operatorss and their filename args

static char **build(char toks[MAX_TOKENS][MAX_LINE], int start, int end) {
    static char *argv[MAX_TOKENS];
    int k = 0;
    for (int i = start; i < end; i++) {
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
static int redirect(char toks[MAX_TOKENS][MAX_LINE], int start, int end, const char *op) {
    for (int i = start; i < end - 1; i++) {
        if (strcmp(toks[i], op) == 0) {
            return i + 1;
        }
    }
    return -1;
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
static void exec_cmd(char **argv) {
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
            }
            exec_cmd(argv);
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
            }
            exec(argv);
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

//segment executor that handles on ; and delim seg and may contain pipes
static void execute_seg(char toks[MAX_TOKENS][MAX_LINE], int start, int end, char prev_line[MAX_LINE]) {
    if (start >= end) {
        return;
    }

    //built in commands
    if (is_builtin(toks[start])) {
        if (strcmp(toks[start], "exit") == 0) {
            printf("Bye bye.\n");
            exit(0);
        } else if (strcmp(toks[start], "cd") == 0) {
            char sub[MAX_TOKENS][MAX_LINE];
            int k = 0;
            for (int i = start; i < end; i++) {
                strncpy(sub[k++], toks[i], MAX_LINE - 1);
            }
            builtin_cd(sub, k);
        } else if (strcmp(toks[start], "source") == 0) {
            char sub[MAX_TOKENS][MAX_LINE];
            int k = 0;
            for (int i = start; i < end; i++) {
                strncpy(sub[k++], toks[i], MAX_LINE - 1);
            }
            builtin_source(sub, k, prev_line);
        } else if (strcmp(toks[start], "help") == 0) {
            builtin_help();
        }
        return;
    }
    execute_pipe(toks, start, end); //external command and pass to pipe chain handlerr
}

//spluts on ; and dispatches each seg
static void execute_command_line(char toks[MAX_TOKENS][MAX_LINE], int ntoks, char prev_line[MAX_LINE]) {
    if (ntoks == 0) {
        return;
    }

    //prev is going to be handered right here so it is never saved as the new prev line
    if (strcmp(toks[0], "prev") == 0) {
        if (prev_line[0] == '\0') {
            return;
        }
        printf("%s\n", prev_line);
        char subtoks[MAX_TOKENS][MAX_LINE];
        int nstoks;
        tokenize(prev_line, subtoks, &nstoks);
        char saved[MAX_LINE];
        strncpy(saved, prev_line, MAX_LINE - 1);
        execute_command_line(subtoks, nstoks, saved);
        return;
    }

    //splits on ; and exec each seg
    int seg_start = 0;
    for (int i = 0; i <= ntoks; i++) {
        if (i == ntoks || strcmp(toks[i], ";") == 0) {
            if (i > seg_start) {
                execute_seg(toks, seg_start, i, prev_line);
            }
            seg_start = i + 1;
        }
    }
}


int main(int argc, char **argv) {
    // TODO: Implement your shell's main
    (void)argc;
    (void)argv;
    printf("Welcome to mini-shell.\n");
    char line[MAX_LINE +2];
    char prev_line[MAX_LINE +1];
    prev_line[0] = '\0';

        while (1) {
        printf("shell $ ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nBye bye.\n"); 
            break;
        }
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n'){
            line[--len] = '\0';
        }
        if (len == 0) continue;
        

        // tokenize
        char toks[MAX_TOKENS][MAX_LINE];
        int ntoks;
        tokenize(line, toks, &ntoks);
        if (ntoks == 0) continue;


        // updates prev line unless the commands is prev
        if (strcmp(toks[0], "prev") != 0) {
            strncpy(prev_line, line, MAX_LINE - 1);
            prev_line[MAX_LINE - 1] = '\0';
        }

        execute_command_line(toks, ntoks, prev_line);
    }
    return 0;
}
