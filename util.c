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
    fprintf(stderr, "segmentation fault occured when analizing token of '%s'.", loc);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  }
  exit(1);
}

bool startswith(char *p, char *q) { return strncmp(p, q, strlen(q)) == 0; }

void expect(char op) {
  if (token->str[0] != op)
    error_at(token->str, "%c not detected", op);
  token = token->next;
}

long expect_number(void) {
  if (token->kind != TK_NUM)
    error_at(token->str, "expected number, but not detected");
  long val = token->val;
  token = token->next;
  return val;
}

char *strTypeOfVar(char *s) {
  int l = strlen(s);
  char *tmp = malloc(l);
  for (int i = 0; i < l; i++) {
    tmp[i] = s[i];
  }
  return tmp;
}

char *strtoalpha(char *s) {
  int l = 0;
  for (int i = 0; s[i] != '\0'; i++) {
    if (isalpha(s[i]) || isdigit(s[i])) {
      l++;
    } else {
      break;
    }
  }
  return strndup(s, l);
}

int lenIsDigit(char *s) {
  int len = 0;
  for (int i = 0; s[i] != '\0'; i++) {
    if (isdigit(s[i])) {
      len++;
    } else {
      break;
    }
  }
  return len;
}
