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
bool startswith(char *p, char *q);
void expect(char op);
long expect_number(void);
char *singleCharToString(char c);
char *strTypeOfVar(char *s, int l);
char *strtoalpha(char *s);
int lenIsDigit(char *s);

//
// tokenize.c
//

// Token
typedef enum {
  TK_RESERVED, // Reserved literal
  TK_NUM,      // Integer literal
  TK_RETURN,   // Return literal
  TK_EOF,      // End-of-file literal
  TK_IDENT,    // Ident literal
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

bool at_eof(void);
Token *consume(char *op);
Token *consume_ident(void);
Token *tokenize(void);

extern char *user_input;
extern Token *token;

//
// codegen.c
//

// Local variable
typedef struct Var Var;
struct Var {
  Var *next;
  char *name; // Variable name
  int offset; // Offset from RBP
};

Var *new_lvar(char *name);

// AST node
typedef enum {
  ND_NUM,    // Integer
  ND_ADD,    //  +
  ND_SUB,    //  -
  ND_MUL,    //  *
  ND_DIV,    //  /
  ND_LPAREN, // (
  ND_RPAREN, // )
  ND_EQ,     // ==
  ND_NEQ,    // !=
  ND_LT,     // <
  ND_LTE,    // <=
  ND_STMT,   // ;
  ND_RETURN, // Return
  ND_IF,     // If
  ND_ELS,    // If
  ND_EOF,    // EOF
  ND_VAR,    // Variable
  ND_ASSIGN, // =
  ND_BLOCK,  // {}
} NodeKind;

// Token type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Token kind
  Node *next;    // Next token
  Token *tok;    // Representative token

  Node *lhs;  // Left-hand side
  Node *rhs;  // Right-hand side
  Node *cond; // condition
  Node *stmt; // statement

  // If
  Node *els;  // else
  Node *then; // then

  // Block
  Node *block;

  Var *var; // Variable

  long val;  // If kind is TK_NUM, its value
  char *str; // Token string
};

typedef struct Function Function;
struct Function {
  Node *node;
  Var *lVars;
  int stack_size;
};

Function *gen_program(void);
void assign_var_offset(Function *function);
void emit_rsp(Function *function);

void code_gen(Node *node);

extern Node *node;
