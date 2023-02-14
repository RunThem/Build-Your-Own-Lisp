#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* clang-format off */
#include <linenoise.h>
#include <mpc.h>
/* clang-format on */

#include "lval.h"

#define LISP_PROMPT "> "

mpc_parser_t *Number, *Symbol, *Sexpr, *Qexpr, *Expr, *Lispy;

void parser() {
  /* Create Some Parsers */
  Number = mpc_new("number");
  Symbol = mpc_new("symbol");
  Sexpr  = mpc_new("sexpr");
  Qexpr  = mpc_new("qexpr");
  Expr   = mpc_new("expr");
  Lispy  = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(
      MPCA_LANG_DEFAULT,
      "number: /-?[0-9]+([.][0-9]+)?/ ;"
      "symbol: '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | \"cons\" ;"
      "sexpr: '(' <expr>* ')' ;"
      "qexpr: '{' <expr>* '}' ;"
      "expr: <number> | <symbol> | <sexpr> | <qexpr> ;"
      "lispy: /^/ <expr>* /$/ ;",
      Number,
      Symbol,
      Sexpr,
      Qexpr,
      Expr,
      Lispy);
}

void clean() {
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
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
      lval* x = lval_read(result.output);
      lval_println(x);
      x = lval_eval(x);
      lval_println(x);
      lval_del(x);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free((void*)input);
  }

  clean();

  return 0;
}
