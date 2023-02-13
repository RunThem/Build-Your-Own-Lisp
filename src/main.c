#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* clang-format off */
#include <linenoise.h>
#include <mpc.h>
/* clang-format on */

#define LISP_PROMPT "> "

mpc_parser_t *Number, *Operator, *Expr, *Lispy;

void parser() {
  /* Create Some Parsers */
  Number   = mpc_new("number");
  Operator = mpc_new("operator");
  Expr     = mpc_new("expr");
  Lispy    = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
            "number: /-?[0-9]+([.][0-9]+)?/ ;"
            "operator: '+' | '-' | '*' | '/' | '%' ;"
            "expr: <number> | '(' <operator> <expr>+ ')' ;"
            "lispy: /^/ <operator> <expr>+ /$/ ;",
            Number,
            Operator,
            Expr,
            Lispy);
}

void clean() {
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
}

int main(int argc, char** argv) {
  printf("lisp!\n");

  parser();

  while (true) {
    const char* input = linenoise(LISP_PROMPT);

    if (input[0] == '\0') {
      continue;
    }

    if (strncmp(input, "quit", sizeof("quit")) == 0) {
      break;
    }

    linenoiseHistoryAdd(input);

    // printf("%*c\'%s\'\n", (int)strlen(LISP_PROMPT), ' ', input);
    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Lispy, &result)) {
      mpc_ast_print(result.output);
      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free((void*)input);
  }

  clean();

  return 0;
}
