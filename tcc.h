#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// util.c
//

void error_at(char *loc, char *fmt, ...);
long expect_number(void);
char *singleCharToString(char c);
char *strTypeOfVar(char *s, int l);
int lenIsDigit(char *s);

//
// tokenize.c
//

// Token
typedef enum {
  TK_NUM,   // Integer literals
  TK_EOF,   // End-of-file markers
  TK_PLUS,  // Plus literals
  TK_MINUS, // Minux literals
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  long val;       // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token length
};

Token *consume(char *op);
Token *tokenize(void);

extern char *user_input;
extern Token *token;

//
// codegen.c
//

// AST node
typedef enum {
  ND_ADD, // num + num
  ND_SUB, // num - num
  ND_NUM, // Integer
  ND_EOF,
} NodeKind;

// Token type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Token kind
  Node *next;    // Next token
  Token *tok;    // Representative token

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  long val;  // If kind is TK_NUM, its value
  char *str; // Token string
};

Node *node_gen();
void code_gen(Node *node);

extern Node *node;
