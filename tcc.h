#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node Node;

//
// util.c
//

void error_at(char *loc, char *fmt, ...);
bool startswith(char *p, char *q);
void expect(char op);
long expect_number(void);
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
Token *expect_str(char *str);
char *expect_ident(void);
Token *consume_ident(void);
Token *tokenize(void);

extern char *user_input;
extern Token *token;

//
// type.c
//

typedef enum {
  TYP_CHAR,
  TYP_INT,
  TYP_CHAR_ARR,
  TYP_INT_ARR,
  TYP_PTR,
} Kind;

// typedef struct Var Var; // from codegen.c
typedef struct Type Type;
struct Type {
  Kind kind;
  int size;   // value of sizeof
  Type *base; // base used when
};

extern Type *typ_char;
extern Type *typ_int;
extern Type *typ_char_arr;
extern Type *typ_int_arr;

bool is_integer(Type *typ);
void typ_rev(Node *node);
bool lr_if_or(Node *node, Kind k);

//
// codegen.c
//

// Local variable
typedef struct Var Var;
struct Var {
  Var *next;
  char *name;    // Variable name
  int offset;    // Offset from RBP
  Type *typ;     // type
  bool is_local; // true if local variable
};

// AST node
typedef enum {
  ND_NUM,       // Integer
  ND_ADD,       //  +
  ND_PTR_ADD,   //  +
  ND_SUB,       //  -
  ND_PTR_SUB,   //  -
  ND_MUL,       //  *
  ND_DIV,       //  /
  ND_EQ,        // ==
  ND_NEQ,       // !=
  ND_LT,        // <
  ND_LTE,       // <=
  ND_ADDR,      // &
  ND_REF,       // *
  ND_RETURN,    // Return
  ND_IF,        // If
  ND_WHILE,     // While
  ND_ELS,       // If
  ND_EOF,       // EOF
  ND_VAR,       // Variable
  ND_FNC,       // Function
  ND_ASSIGN,    // =
  ND_GNU_BLOCK, // ({})
  ND_BLOCK,     // { }
  ND_NULL,      // null
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

  Node *args; // Function

  // If
  Node *els;  // else
  Node *then; // then

  // Block
  Node *block;

  Var *var; // Variable

  long val;  // If kind is TK_NUM, its value
  char *str; // Token string

  Type *typ; // type of node.
};

typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  Node *node;
  Var *lVars;
  int stack_size;
};

typedef struct Program Program;
struct Program {
  Function *func;
  Var *gVars;
};

Program *gen_program(void);
void assign_var_offset(Function *function);
void emit_rsp(Function *function);

void code_gen(Node *node);

extern Node *node;
