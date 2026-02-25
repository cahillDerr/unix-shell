
#define Max_Line 255



static int tokenize(const char *src, char toks[Max_Tokens][Max_Line], int *ntoks) {
  *ntoks = 0;
  int i = 0;
  int len = (int)strlen(src);

  while (i < len && *ntoks < Max_Tokens - 1) {
    while (i < len && (src[i] == ' ' || src[i] == '\t')) i++;
    if (i >= len) break;
    int j = 0;
    if src[i] == '"') {
      i++;
      while ( i < len && src[i] != '"')
          toks[*ntoks][]


    }

  }
}

int main(int argc, char **argv) {

  // TODO: Implement your shell's main
  (void) argc;
  (void) argv;
  print("Welcome to mini-shell.\n")

  while (1) {
    printf("shell $");
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL) {
      printf("\nBye Bye.\n")
      break;
    }
    size_t len = strlen(line);
    
  }




}
