#ifndef LVAL_H
#define LVAL_H

#include <mpc.h>
#include <stdint.h>

/* Declare New lvl Struct */
typedef struct lval {
  enum {
    LVAL_INTEGER,
    LVAL_DECIMAL,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR
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

lval* lval_integer(int64_t x);
lval* lval_err(const char* m);
lval* lval_sym(const char* m);
lval* lval_sexpr();
lval* lval_qexpr();

lval* lval_add(lval* v, lval* x);
void lval_del(lval* v);

lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);

void lval_print(lval* v);
void lval_println(lval* v);

lval* lval_evel_sexpr(lval* v);
lval* lval_eval(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);

#endif /* end of include guard: LVAL_H */
