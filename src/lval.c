#include "lval.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lval* lval_integer(int64_t x) {
  lval* v    = (lval*)calloc(1, sizeof(lval));
  v->type    = LVAL_INTEGER;
  v->integer = x;

  return v;
}

lval* lval_decimal(double x) {
  lval* v    = (lval*)calloc(1, sizeof(lval));
  v->type    = LVAL_DECIMAL;
  v->decimal = x;

  return v;
}

lval* lval_sym(const char* m) {
  lval* v = (lval*)calloc(1, sizeof(lval));
  v->type = LVAL_SYM;
  v->sym  = strdup(m);

  return v;
}

lval* lval_sexpr() {
  lval* v  = (lval*)calloc(1, sizeof(lval));
  v->type  = LVAL_SEXPR;
  v->count = 0;
  v->cell  = NULL;

  return v;
}

lval* lval_err(const char* m) {
  lval* v = (lval*)calloc(1, sizeof(lval));
  v->type = LVAL_ERR;
  v->err  = strdup(m);

  return v;
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell               = realloc(v->cell, v->count * sizeof(lval*));
  v->cell[v->count - 1] = x;

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

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  if (strchr(t->contents, '.')) {
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_decimal(x) : lval_err("invalid number!");
  } else {
    int64_t x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_integer(x) : lval_err("invalid number!");
  }
}

lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return lval_read_num(t);
  }
  if (strstr(t->tag, "symbol")) {
    return lval_sym(t->contents);
  }

  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) {
    x = lval_sexpr();
  }
  if (strstr(t->tag, "sexpr")) {
    x = lval_sexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0 || strcmp(t->children[i]->contents, ")") == 0 ||
        strcmp(t->children[i]->contents, "{") == 0 || strcmp(t->children[i]->contents, "}") == 0) {
      continue;
    }

    lval* p = lval_read(t->children[i]);
    if (p == NULL) {
      continue;
    }

    x = lval_add(x, p);
  }

  return x;
}

/* Print an "Lval" */
void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_INTEGER:
      printf("%li", v->integer);
      break;
    case LVAL_ERR:
      printf("Error: %s", v->err);
      break;
    case LVAL_SYM:
      printf("%s", v->sym);
      break;
    case LVAL_SEXPR:
      printf("(");
      for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != (v->count - 1)) {
          printf(" ");
        }
      }
      printf(")");
      break;

    default:
      break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  printf("\n");
}

lval* eval_op(lval* x, char* op, lval* y) {
  if (x->type == LVAL_ERR) {
    return x;
  }

  if (y->type == LVAL_ERR) {
    return y;
  }

  if (x->type == LVAL_DECIMAL || y->type == LVAL_DECIMAL) {
    if (strcmp(op, "+") == 0) {
      return lval_decimal(x->decimal + y->decimal);
    }
    if (strcmp(op, "-") == 0) {
      return lval_decimal(x->decimal - y->decimal);
    }
    if (strcmp(op, "*") == 0) {
      return lval_decimal(x->decimal * y->decimal);
    }
    if (strcmp(op, "/") == 0) {
      return (y->decimal == 0) ? lval_err("division by zero!") :
                                 lval_decimal(x->decimal / y->decimal);
    }
    if (strcmp(op, "^") == 0) {
      return lval_decimal(pow(x->decimal, y->decimal));
    }
    if (strcmp(op, "min") == 0) {
      return x->decimal < y->decimal ? x : y;
    }
    if (strcmp(op, "max") == 0) {
      return x->decimal > y->decimal ? x : y;
    }
  } else {
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
      return (y->integer == 0) ? lval_err("division by zero!") :
                                 lval_integer(x->integer / y->integer);
    }
    if (strcmp(op, "%") == 0) {
      return (y->integer == 0) ? lval_err("division by zero!") :
                                 lval_integer(x->integer % y->integer);
    }
    if (strcmp(op, "^") == 0) {
      return lval_integer((int64_t)pow(x->integer, y->integer));
    }
    if (strcmp(op, "min") == 0) {
      return x->integer < y->integer ? x : y;
    }
    if (strcmp(op, "max") == 0) {
      return x->integer > y->integer ? x : y;
    }
  }

  return lval_err("invalid operator!");
}
