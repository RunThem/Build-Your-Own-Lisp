#ifndef LVAL_H
#define LVAL_H

#include <mpc.h>
#include <stdint.h>

struct lenv;
struct lval;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval* (*lbuiltin)(lenv*, lval*);

/* Declare New lvl Struct */
struct lval {
  enum {
    LVAL_INTEGER,
    LVAL_DECIMAL,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN
  } type;

  lbuiltin fun;

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
};

struct lenv {
  int count;
  char** syms;
  lval** vals;
};


lval* lval_integer(int64_t x);
lval* lval_decimal(double x);
lval* lval_sym(const char* m);
lval* lval_sexpr();
lval* lval_qexpr();
lval* lval_fun(lbuiltin func);
lval* lval_err(const char* m);
lval* lval_add(lval* v, lval* x);
lval* lval_copy(lval* v);
void lval_del(lval* v);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
void lval_print(lval* v);
void lval_println(lval* v);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* lval_join(lval* x, lval* y);
lval* builtin_join(lval* a);
lval* builtin_cons(lval* a);
lval* builtin_len(lval* a);
lval* builtin_init(lval* a);
lval* builtin_op(lval* a, char* op);
lval* builtin(lval* a, char* func);
lval* lval_evel_sexpr(lenv* e, lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_eval(lenv* e, lval* v);
lenv* lenv_new();
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);


#endif /* end of include guard: LVAL_H */
