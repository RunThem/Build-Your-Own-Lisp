#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* clang-format off */
#include <linenoise.h>
#include <mpc.h>
/* clang-format on */

#define LISP_PROMPT "=> "

int main(int argc, char** argv) {
  printf("lisp!\n");

  while (true) {
    const char* input = linenoise(LISP_PROMPT);

    if (input[0] == '\0') {
      continue;
    }

    if (strncmp(input, "quit", sizeof("quit")) == 0) {
      break;
    }

    linenoiseHistoryAdd(input);

    printf("%*c\'%s\'\n", (int)strlen(LISP_PROMPT), ' ', input);

    free((void*)input);
  }

  return 0;
}
