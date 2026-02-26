#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 255
#define MAX_TOKENS 128

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

// exec argv on failure and print serror then exits
//only actually called from inside achild process. 
static void exec(char **argv[]) {
    execvp(argv[0], argv);
    fprintf(stderr, "%s: command not found\n", argv[0]);
    exit(1);
}

static void execute_pipe(char toks[MAX_TOKENS][MAX_LINE], int start, int end) {
    int seg_start = [MAX_TOKENS];
    int seg_end = [MAX_TOKENS];
    int nseg = 0;
    int s = start;
    for (i <= start; i <= end; i++) {
        if (i == end || strcmp (toks[i], "|") == 0) {
            seg_start[nseg] = s;
            seg_end[nseg] = i;
            nseg++;
            s = i + 1;
    }
    //d
    if (nseg == 1) {
        pid_t pid = fork(); //single command  no pipe
        if (pid < 0) {
            perror("fork");
            return; 
        }
        if (pid == 0){
            redirect(toks, start, end);
            char **argv = build(toks, start, end);
            if (argv[0] == NULL) {
                exit(0);
                exec(argv);
            }
            waitpid(pid, NULL, 0);
            return;
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
