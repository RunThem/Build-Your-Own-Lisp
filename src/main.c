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

#define LISP_PROMPT "> "

/* Declare New lvl Struct */
typedef struct {
  enum {
    LVAL_NUM,
    LVAL_ERR
  } type;

  union {
    int64_t num;
    enum {
      LERR_DIV_ZERO,
      LERR_BAD_OP,
      LERR_BAD_NUM,
    } err;
  };
} Lval;

mpc_parser_t *Number, *Operator, *Expr, *Lispy;

Lval lval_num(int64_t x) {
  Lval v = {
      .type = LVAL_NUM,
      .num  = x,
  };

  return v;
}

Lval lval_err(int x) {
  Lval v = {
      .type = LVAL_ERR,
      .err  = x,
  };

  return v;
}

/* Print an "Lval" */
void lval_print(Lval v) {
  switch (v.type) {
    case LVAL_NUM:
      printf("%li", v.num);
      break;
    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division by zero!");
      } else if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid operator!");
      } else if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid Number!");
      }
      break;

    default:
      break;
  }
}

/* Print an "Lval" followed by a newline. */
void lval_println(Lval v) {
  lval_print(v);
  printf("\n");
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

Lval eval_op(Lval x, char* op, Lval y) {
  if (x.type == LVAL_ERR) {
    return x;
  }

  if (y.type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    return lval_num(x.num + y.num);
  }
  if (strcmp(op, "-") == 0) {
    return lval_num(x.num - y.num);
  }
  if (strcmp(op, "*") == 0) {
    return lval_num(x.num * y.num);
  }
  if (strcmp(op, "/") == 0) {
    return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  if (strcmp(op, "%") == 0) {
    return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);
  }
  if (strcmp(op, "^") == 0) {
    return lval_num(pow(x.num, y.num));
  }
  if (strcmp(op, "min") == 0) {
    return x.num < y.num ? x : y;
  }
  if (strcmp(op, "max") == 0) {
    return x.num > y.num ? x : y;
  }

  return lval_err(LERR_BAD_OP);
}

Lval eval(mpc_ast_t* t) {
  /* If tagged as number retruen it directly. */
  if (strstr(t->tag, "number")) {
    errno  = 0;
    long x = strtol(t->contents, NULL, 10);
    return (errno != ERANGE) ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x`. */
  Lval x = eval(t->children[2]);

  /* Unary operator */
  if (t->children_num == 4) {
    x.num = -x.num;
    return x;
  }

  for (int i = 3; strstr(t->children[i]->tag, "expr"); i++) {
    x = eval_op(x, op, eval(t->children[i]));
  }

  return x;
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
      lval_println(eval(result.output));
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
