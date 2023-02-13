#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* clang-format off */
#include <linenoise.h>
#include <mpc.h>
/* clang-format on */

#define LISP_PROMPT "=> "

void test_mpc() {
  mpc_parser_t* Adjective = mpc_new("adjective");
  mpc_parser_t* Noun      = mpc_new("noun");
  mpc_parser_t* Phrase    = mpc_new("phrase");
  mpc_parser_t* Doge      = mpc_new("doge");

  // 彩蛋
  //
  // number: <integer> '.' <float>;
  // integer: [0-9][1-9]*;
  // float: [1-9];
  //
  // url: <scheme> "://" <realm_name> <path>
  // scheme: "https" | "http"
  // realm_name: <word> '.' <word> ['.' <word>]*
  // path: '/' | '/' <word> ['/' <word>]*
  // word: [a-zA-Z]+
  //
  // not parse JSON
  mpca_lang(MPCA_LANG_DEFAULT,
            "adjective: \"wow\" | \"many\" | \"so\" | \"such\";"
            "noun: \"lisp\" | \"language\" | \"book\" | \"build\" | \"c\";"
            "phrase: <adjective> <noun>;"
            "doge: <phrase>*;",
            Adjective,
            Noun,
            Phrase,
            Doge);

  mpc_cleanup(4, Adjective, Noun, Phrase, Doge);
}

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
