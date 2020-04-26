#include "tcc.h"

char *user_input;
Token *token;

// Consumes the current token if it matches `op`.
Token *consume(char *op) {
  if (strlen(op) != token->len ||
      strncmp(token->str, op, token->len))
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

  while (*p) {

    // WhiteSpace literal
    if (isspace(*p)) {
      p++;
      continue;
    };

    // Add literal
    if (*p == '+') {
      p++;
      cur = new_token(TK_PLUS, cur, p, 1);
      continue;
    }

    // Sub literal
    if (*p == '-') {
      p++;
      cur = new_token(TK_MINUS, cur, p, 1);
      continue;
    }

    // Integer literal
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    };
  }
  cur = new_token(TK_EOF, cur, p, 1);
  return head.next;
}
