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

lval* lval_qexpr() {
  lval* v  = (lval*)calloc(1, sizeof(lval));
  v->type  = LVAL_QEXPR;
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
    case LVAL_QEXPR:
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
  if (strstr(t->tag, "qexpr")) {
    x = lval_qexpr();
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
    case LVAL_DECIMAL:
      printf("%f", v->decimal);
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
    case LVAL_QEXPR:
      printf("{");
      for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != (v->count - 1)) {
          printf(" ");
        }
      }
      printf("}");
      break;

    default:
      break;
  }
}

void lval_println(lval* v) {
  lval_print(v);
  printf("\n");
}

#define lassert(args, cond, err)                                                                   \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      lval_del(args);                                                                              \
      return lval_err(err);                                                                        \
    }                                                                                              \
  } while (0);

lval* builtin_head(lval* a) {
  lassert(a, a->count == 1, "function 'head' passed too many arguments!");
  lassert(a, a->cell[0]->type == LVAL_QEXPR, "function 'head' passed incorrect types!");
  lassert(a, a->cell[0]->count != 0, "function 'head' passed {}!");

  lval* v = lval_take(a, 0);
  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_tail(lval* a) {
  lassert(a, a->count == 1, "function 'tail' passed too many arguments!");
  lassert(a, a->cell[0]->type == LVAL_QEXPR, "function 'tail' passed incorrect types!");
  lassert(a, a->cell[0]->count != 0, "function 'tail' passed {}!");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));

  return v;
}

lval* builtin_list(lval* a) {
  a->type = LVAL_QEXPR;

  return a;
}

lval* builtin_eval(lval* a) {
  lassert(a, a->count == 1, "function 'eval' passed too many arguments!");
  lassert(a, a->cell[0]->type == LVAL_QEXPR, "fcuntion 'eval' passed incorrect type!");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;

  return lval_eval(x);
}

lval* lval_join(lval* x, lval* y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  lval_del(y);
  return x;
}

lval* builtin_join(lval* a) {
  for (int i = 0; i < a->count; i++) {
    lassert(a, a->cell[i]->type == LVAL_QEXPR, "function 'join' passed incorrect type.");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_cons(lval* a) {
  lassert(a, a->count == 2, "function 'cons' passed too many arguments!");
  lassert(a,
          a->cell[0]->type == LVAL_DECIMAL || a->cell[0]->type == LVAL_INTEGER,
          "function 'cons' passed incorrect type!");
  lassert(a, a->cell[1]->type == LVAL_QEXPR, "fcuntion 'cons' passed incorrect type!");

  lval* x = lval_qexpr();
  lval_add(x, lval_pop(a, 0));
  lval* y = lval_pop(a, 0);

  lval_add(a, x);
  lval_add(a, y);

  return builtin_join(a);
}

lval* builtin_op(lval* a, char* op) {
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_INTEGER && a->cell[i]->type != LVAL_DECIMAL) {
      lval_del(a);
      return lval_err("cannot operate on non-number!");
    }
  }

  lval* x = lval_pop(a, 0);

  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->integer = -x->integer;
  }

  while (a->count > 0) {
    lval* y = lval_pop(a, 0);

#define is_integer(x, y) ((x)->type == LVAL_INTEGER && (y)->type == LVAL_INTEGER)
#define num_of(x)        ((x)->type == LVAL_INTEGER ? (x)->integer : (x)->decimal)

    if (strcmp(op, "+") == 0) {
      if (is_integer(x, y)) {
        x->integer += y->integer;
      } else {
        x->decimal = num_of(x) + num_of(y);
        x->type    = LVAL_DECIMAL;
      }
    } else if (strcmp(op, "-") == 0) {
      if (is_integer(x, y)) {
        x->integer -= y->integer;
      } else {
        x->decimal = num_of(x) - num_of(y);
        x->type    = LVAL_DECIMAL;
      }
    } else if (strcmp(op, "*") == 0) {
      if (is_integer(x, y)) {
        x->integer *= y->integer;
      } else {
        x->decimal = num_of(x) * num_of(y);
        x->type    = LVAL_DECIMAL;
      }
    } else if (strcmp(op, "/") == 0) {
      if (num_of(y) == 0.0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("division by zero!");
        break;
      } else {
        x->decimal = num_of(x) / num_of(y);
        x->type    = LVAL_DECIMAL;
      }
    } else if (strcmp(op, "%") == 0) {
      if (is_integer(x, y)) {
        if (y->integer == 0) {
          lval_del(x);
          lval_del(y);
          x = lval_err("division by zero!");
          break;
        } else {
          x->integer %= y->integer;
        }
      } else {
        x = lval_err("cannot operate on non-number!");
      }
    } else if (strcmp(op, "^") == 0) {
      if (is_integer(x, y)) {
        x->integer = (int64_t)pow(x->integer, y->integer);
      } else {
        x->decimal = pow(num_of(x), num_of(y));
        x->type    = LVAL_DECIMAL;
      }
    } else if (strcmp(op, "min") == 0) {
      if (num_of(x) > num_of(y)) {
        if (y->type == LVAL_INTEGER) {
          x->integer = y->integer;
        } else {
          x->decimal = y->decimal;
        }
        x->type = y->type;
      }
    } else if (strcmp(op, "max") == 0) {
      if (num_of(x) < num_of(y)) {
        if (y->type == LVAL_INTEGER) {
          x->integer = y->integer;
        } else {
          x->decimal = y->decimal;
        }
        x->type = y->type;
      }
    }

    lval_del(y);
  }

  lval_del(a);

  return x;
}

lval* builtin(lval* a, char* func) {
  if (strcmp(func, "list") == 0) {
    return builtin_list(a);
  } else if (strcmp(func, "head") == 0) {
    return builtin_head(a);
  } else if (strcmp(func, "tail") == 0) {
    return builtin_tail(a);
  } else if (strcmp(func, "join") == 0) {
    return builtin_join(a);
  } else if (strcmp(func, "eval") == 0) {
    return builtin_eval(a);
  } else if (strcmp(func, "cons") == 0) {
    return builtin_cons(a);
  } else {
    return builtin_op(a, func);
  }

  lval_del(a);

  return lval_err("unknown function!");
}

lval* lval_evel_sexpr(lval* v) {
  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /* Error checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  /* Empty expression */
  if (v->count == 0) {
    return v;
  }

  /* Single expression */
  if (v->count == 1) {
    return lval_take(v, 0);
  }

  /* Ensure first element is symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression does not start with symbol!");
  }

  lval* result = builtin(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_pop(lval* v, int i) {
  lval* x = v->cell[i];

  memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));
  v->count--;

  v->cell = realloc(v->cell, sizeof(lval*) * v->count);

  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);

  return x;
}

lval* lval_eval(lval* v) {
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) {
    return lval_evel_sexpr(v);
  }

  /* All other lval types remain the same */
  return v;
}
