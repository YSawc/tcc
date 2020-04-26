#include "tcc.h"

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  if (0 <= pos && pos <= 256) {
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  } else {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  }
  exit(1);
}

long expect_number(void) {
  if (token->kind != TK_NUM)
    error_at(token->str, "expected number, but not detected");
  long val = token->val;
  token = token->next;
  return val;
}
