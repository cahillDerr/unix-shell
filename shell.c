
#define Max_Line 255



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
