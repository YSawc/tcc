#include "tcc.h"

char *filename;
char *user_input;
Token *token;

bool at_eof() { return token->kind == TK_EOF; }

// Consumes the current token if it matches `op`.
Token *consume(char *op) {
  if (!token) {
    fprintf(stderr, "parse token not detected.");
    exit(1);
  }
  if (strlen(op) != token->len || strncmp(token->str, op, token->len))
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

Token *expect_str(char *str) {
  if (strlen(str) != token->len || strncmp(token->str, str, token->len))
    error_at(str, "%s expected.");
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

// Expect ident.
char *expect_ident(void) {
  if (token->kind != TK_IDENT)
    error_at(token->str, "Expect ident token.");
  char *s = token->str;
  token = token->next;
  return s;
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

static char get_escape_char(char c) {
  switch (c) {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'v':
    return '\v';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  case 'e':
    return 27;
  case '0':
    return 0;
  default:
    return c;
  }
}

Token *read_typed_str(Token *cur, char *p) {
  int len = 0;
  char *q = p;
  char buf[256];
  while (*p && *p != '"') {
    if (!*p) {
      error_at(cur->str, "string literal must close with \".");
    }
    if (*p == '\0')
      error_at(cur->str, "unclosed string literal");

    if (*p == '\\') {
      p++;
      buf[len++] = get_escape_char(*p++);
    } else {
      buf[len++] = *p++;
    }
  }
  Token *tok = new_token(TK_STR, cur, NULL, p - q);
  tok->str = malloc(len+1);
  memcpy(tok->str, buf, len);
  tok->len = p - q;
  cur->next = tok;
  return tok;
}

Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  // list of reserved multiple .symbol
  static char *mSym[] = {"==", "!=", "<=", "=>", ">=", "=<"};

  // list of reserved single letter.symbol
  static char sSym[] = {'+', '-', '*', '&', '/', '(', ')', '{', '}',
                        '[', ']', ';', '=', '<', '>', ',', '\''};

  // list of reserved multiple letter.string
  static char *mSt[] = {"if", "else", "while", "int", "char", "sizeof"};

  while (*p) {

    // WhiteSpace literal
    if (isspace(*p)) {
      p++;
      continue;
    };

    if (*p == '"') {
      p++;
      cur = read_typed_str(cur, p);
      p += cur->len + 1;
      continue;
    };

    // Skip line comments.
    if (startswith(p, "//")) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    // Skip block comments.
    if (startswith(p, "/*")) {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at(p, "unclosed block comment");
      p = q + 2;
      continue;
    }

    // Detector in list of reserved multiple letter
    for (int i = 0; i < sizeof(mSym) / sizeof(*mSym); i++) {
      int tmp_len = strlen(mSym[i]);
      if (startswith(p, mSym[i])) {
        cur = new_token(TK_RESERVED, cur, strTypeOfVar(mSym[i]), tmp_len);
        p += tmp_len;
        continue;
      }
    }

    // Detector in list of reserved single letter
    for (int i = 0; i < sizeof(sSym) / sizeof(*sSym); i++) {
      if (*p == sSym[i]) {
        cur = new_token(TK_RESERVED, cur, p, 1);
        p++;
        continue;
      }
    }

    // Detector in list of reserved multiple letter string
    for (int i = 0; i < sizeof(mSt) / sizeof(*mSt); i++) {
      int tmp_len = strlen(mSt[i]);
      if (startswith(p, mSt[i])) {
        cur = new_token(TK_RESERVED, cur, strTypeOfVar(mSt[i]), tmp_len);
        p += tmp_len;
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
      cur = new_token(TK_NUM, cur, strTypeOfVar(p), l);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    };
  }
  cur = new_token(TK_EOF, cur, p, 1);
  return head.next;
}
