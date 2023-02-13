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
typedef struct lval {
  enum {
    LVAL_INTEGER,
    LVAL_DECIMAL,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR
  } type;

  union {
    int64_t integer;
    double decimal;

    char* err;
    char* sym;

    struct {
      int count;
      struct lval** cell;
    };
  };
} lval;

mpc_parser_t *Number, *Operator, *Expr, *Lispy;

/* Construct a pointer to a new Integer lval. */
lval* lval_integer(int64_t x) {
  lval* v    = (lval*)calloc(1, sizeof(lval));
  v->type    = LVAL_INTEGER;
  v->integer = x;

  return v;
}

/* Construct a pointer to a new Error lval */
lval* lval_err(const char* m) {
  lval* v = (lval*)calloc(1, sizeof(lval));
  v->type = LVAL_ERR;
  v->err  = strdup(m);

  return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(const char* m) {
  lval* v = (lval*)calloc(1, sizeof(lval));
  v->type = LVAL_SYM;
  v->sym  = strdup(m);

  return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr() {
  lval* v  = (lval*)calloc(1, sizeof(lval));
  v->type  = LVAL_SEXPR;
  v->count = 0;
  v->cell  = NULL;

  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
    case LVAL_INTEGER:
      break;
    case LVAL_DECIMAL:
      break;
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_SYM:
      free(v->sym);
      break;
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }

      free(v->cell);
      break;
    default:
      break;
  }

  free(v);
}

/* Print an "Lval" */
void lval_println(lval* v) {
  switch (v->type) {
    case LVAL_INTEGER:
      printf("%li", v->integer);
      break;
    case LVAL_ERR:
      printf("%s", v->err);
      break;

    default:
      break;
  }

  printf("\n");
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

lval* eval_op(lval* x, char* op, lval* y) {
  if (x->type == LVAL_ERR) {
    return x;
  }

  if (y->type == LVAL_ERR) {
    return y;
  }

  if (strcmp(op, "+") == 0) {
    return lval_integer(x->integer + y->integer);
  }
  if (strcmp(op, "-") == 0) {
    return lval_integer(x->integer - y->integer);
  }
  if (strcmp(op, "*") == 0) {
    return lval_integer(x->integer * y->integer);
  }
  if (strcmp(op, "/") == 0) {
    return (y->integer == 0) ? lval_err("Error: Division by zero!") :
                               lval_integer(x->integer / y->integer);
  }
  if (strcmp(op, "%") == 0) {
    return (y->integer == 0) ? lval_err("Error: Division by zero!") :
                               lval_integer(x->integer % y->integer);
  }
  if (strcmp(op, "^") == 0) {
    return lval_integer(pow(x->integer, y->integer));
  }
  if (strcmp(op, "min") == 0) {
    return x->integer < y->integer ? x : y;
  }
  if (strcmp(op, "max") == 0) {
    return x->integer > y->integer ? x : y;
  }

  return lval_err("Error: Invalid operator!");
}

lval* eval(mpc_ast_t* t) {
  /* If tagged as number retruen it directly. */
  if (strstr(t->tag, "number")) {
    errno  = 0;
    long x = strtol(t->contents, NULL, 10);
    return (errno != ERANGE) ? lval_integer(x) : lval_err("Error: Invalid Number!");
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x`. */
  lval* x = eval(t->children[2]);

  /* Unary operator */
  if (t->children_num == 4) {
    x->integer = -x->integer;
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
