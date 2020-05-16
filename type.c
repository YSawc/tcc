#include "tcc.h"

Type *typ_char = &(Type){TYP_CHAR, 1};
Type *typ_int = &(Type){TYP_INT, 8};
Type *typ_char_arr = &(Type){TYP_CHAR_ARR, 1};
Type *typ_int_arr = &(Type){TYP_INT_ARR, 8};
Type *typ_d_by = &(Type){TYP_D_BY, 1};

bool is_integer(Type *typ) { return typ->kind == TYP_INT; }

Type *pointer_to(Type *base) {
  Type *typ = calloc(1, sizeof(Type));
  typ->kind = TYP_PTR;
  typ->base = base;
  return typ;
}

// Boolean formed intermediate within LR or.
bool lr_if_or(Node *node, Kind k) {
  return node->typ->kind == k || k == node->rhs->typ->kind;
}

void typ_rev(Node *node) {
  if (!node || node->typ)
    return;

  typ_rev(node->lhs);
  typ_rev(node->rhs);
  typ_rev(node->cond);
  typ_rev(node->stmt);

  typ_rev(node->args);

  typ_rev(node->els);
  typ_rev(node->then);

  for (Node *n = node->args; n; n = n->next)
    typ_rev(n);
  for (Node *n = node->block; n; n = n->next)
    typ_rev(n);

  switch (node->kind) {
  case ND_NUM:
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NEQ:
  case ND_LT:
  case ND_LTE:
  case ND_VAR:
  case ND_FNC:
    node->typ = typ_int;
    return;
  case ND_PTR_ADD:
  case ND_PTR_SUB:
  case ND_ASSIGN:
    node->typ = node->lhs->typ;
    if (node->lhs->var && node->rhs->typ == typ_char)
      node->lhs->var = node->rhs->var;
    return;
  case ND_ADDR:
    node->typ = pointer_to(node->lhs->typ);
    return;
  case ND_REF:
    node->typ = node->lhs->typ;
    return;
  default:
    return;
  }
}
