#include "tcc.h"

char *user_input;
Token *token;

bool at_eof() { return token->kind == TK_EOF; }

// Consumes the current token if it matches `op`.
Token *consume(char *op) {
  if (strlen(op) != token->len || strncmp(token->str, op, token->len))
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// Consumes the current token if it is an identifier.
Token *consume_ident(void) {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// create new token
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token)); // flag with prologue.
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  // list of reserved multiple letter.
  static char *rmw[] = {"==", "!=", "<=", "=>", ">=", "=<", "if", "else"};

  // list of reserved single letter.
  static char rsw[] = {
      '+', '-', '*', '/', '(', ')', ';', '=', '<', '>',
  };

  while (*p) {

    // WhiteSpace literal
    if (isspace(*p)) {
      p++;
      continue;
    };

    // Detector in list of reserved multiple letter
    for (int i = 0; i < sizeof(rmw) / sizeof(*rmw); i++) {
      int tmp_len = strlen(rmw[i]);
      if (startswith(p, rmw[i])) {
        cur = new_token(TK_RESERVED, cur, strTypeOfVar(p, tmp_len), tmp_len);
        p += tmp_len;
        continue;
      }
    }

    // Detector in list of reserved single letter
    for (int i = 0; i < sizeof(rsw) / sizeof(*rsw); i++) {
      if (*p == rsw[i]) {
        cur = new_token(TK_RESERVED, cur, p, 1);
        p++;
        continue;
      }
    }

    // alpha literal
    if (isalpha(*p)) {
      char *tmp = strtoalpha(p);
      p += strlen(tmp);
      cur = new_token(TK_IDENT, cur, tmp, strlen(tmp));
      continue;
    }

    // Integer literal
    if (isdigit(*p)) {
      int l = lenIsDigit(p);
      cur = new_token(TK_NUM, cur, strTypeOfVar(p, l), l);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    };
  }
  cur = new_token(TK_EOF, cur, p, 1);
  return head.next;
}
