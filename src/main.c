#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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
            "operator: '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;"
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

int64_t eval_op(int64_t x, char* op, int64_t y) {
  if (strcmp(op, "+") == 0) {
    return x + y;
  }
  if (strcmp(op, "-") == 0) {
    return x - y;
  }
  if (strcmp(op, "*") == 0) {
    return x * y;
  }
  if (strcmp(op, "/") == 0) {
    return x / y;
  }
  if (strcmp(op, "%") == 0) {
    return x % y;
  }
  if (strcmp(op, "^") == 0) {
    return pow(x, y);
  }
  if (strcmp(op, "min") == 0) {
    return x < y ? x : y;
  }
  if (strcmp(op, "max") == 0) {
    return x > y ? x : y;
  }

  return 0;
}

int64_t eval(mpc_ast_t* t) {
  /* If tagged as number retruen it directly. */
  if (strstr(t->tag, "number")) {
    return atol(t->contents);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x`. */
  int64_t x = eval(t->children[2]);

  for (int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
    x = eval_op(x, op, eval(t->children[i]));
  }

  return x;
}

int number_of_nodes(mpc_ast_t* t) {
  if (t->children_num == 0) {
    return 1;
  }

  int total = 1;
  for (int i = 0; i < t->children_num; i++) {
    total += number_of_nodes(t->children[i]);
  }

  return total;
}

int number_of_leaf_nodes(mpc_ast_t* t) {
  if (t->children_num == 0) {
    return 1;
  }

  int total = 0;
  for (int i = 0; i < t->children_num; i++) {
    total += number_of_leaf_nodes(t->children[i]);
  }

  return total;
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
      int64_t n = eval(result.output);
      printf("%li\n", n);
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
