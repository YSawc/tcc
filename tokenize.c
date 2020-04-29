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

  while (*p) {

    // WhiteSpace literal
    if (isspace(*p)) {
      p++;
      continue;
    };

    // Add literal
    if (*p == '+') {
      cur = new_token(TK_PLUS, cur, "+", 1);
      p++;
      continue;
    }

    // Sub literal
    if (*p == '-') {
      cur = new_token(TK_MINUS, cur, "-", 1);
      p++;
      continue;
    }

    // Asterisk literal
    if (*p == '*') {
      cur = new_token(TK_ASTERISC, cur, "*", 1);
      p++;
      continue;
    }

    // Slash literal
    if (*p == '/') {
      cur = new_token(TK_SLASH, cur, "/", 1);
      p++;
      continue;
    }

    // LParen literal
    if (*p == '(') {
      cur = new_token(TK_LPAREN, cur, "(", 1);
      p++;
      continue;
    }

    // RParen literal
    if (*p == ')') {
      cur = new_token(TK_RPAREN, cur, ")", 1);
      p++;
      continue;
    }

    // RParen literal
    if (*p == ')') {
      cur = new_token(TK_RPAREN, cur, ")", 1);
      p++;
      continue;
    }

    // RParen literal
    if (*p == ';') {
      cur = new_token(TK_STMT, cur, ";", 1);
      p++;
      continue;
    }

    // Comparison literal
    if ((startswith(p, "==")) || (startswith(p, "!=")) ||
        (startswith(p, "<=")) || (startswith(p, "=>")) ||
        (startswith(p, ">=")) || (startswith(p, "=<"))) {
      cur = new_token(TK_COMPARISON, cur, strTypeOfVar(p, 2), 2);
      p += 2;
      continue;
    }

    // Assign literal
    if (*p == '=') {
      cur = new_token(TK_ASSIGN, cur, "=", 1);
      p++;
      continue;
    }

    // Comparison literal
    if ((*p == '<') || (*p == '>')) {
      cur = new_token(TK_COMPARISON, cur, strTypeOfVar(p, 1), 1);
      p++;
      continue;
    }

    // return literal
    if (startswith(p, "return")) {
      cur = new_token(TK_RETURN, cur, strTypeOfVar(p, 6), 6);
      p += 6;
      continue;
    }

    // alpha literal
    if (isalpha(*p)) {
      char *tmp = strtoalpha(p);
      /* Var *var = find_var(tmp); */
      /* if (!var) { */
      /*   var = new_lvar(tmp); */
      /*   var = var->next; */
      /* } */
      p += strlen(tmp);
      cur = new_token(TK_IDENT, cur, tmp, strlen(tmp));
      continue;
    }

    // Integer literal
    if (isdigit(*p)) {
      int l = lenIsDigit(p);
      cur = new_token(TK_NUM, cur, strTypeOfVar(p, l), 1);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    };

    // Identifier
    /* if ('a' <= *p && *p <= 'z') { */
    /*   cur = new_token(TK_IDENT, cur, strTypeOfVar(p, 1), 1); */
    /*   p++; */
    /*   continue; */
    /* } */
  }
  cur = new_token(TK_EOF, cur, p, 1);
  return head.next;
}
