#include "tcc.h"

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error message in the following format and exit.
//
// foo.c:10: x = y + 1;
//               ^ <error message here>
static void verror_at(char *loc, char *fmt, va_list ap) {

  // Find a line containing `loc`.
  fprintf(stderr, "%s\n", user_input);
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  // Get a line number.
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  // Print out the line.
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // Show the error message.
  int pos = loc - line + indent;
  if (0 <= pos && pos <= 256) {
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "segmentation fault occured when analizing token of '%s'.",
            loc);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
  }
  exit(1);
}

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
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
