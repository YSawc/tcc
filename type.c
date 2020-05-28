#include "tcc.h"

Type *ty_char = &(Type){TY_CHAR, 1};
Type *ty_int = &(Type){TY_INT, 8};
Type *ty_b = &(Type){TY_B, 1};
Type *ty_char_arr = &(Type){TY_CHAR_ARR, 1};
Type *ty_int_arr = &(Type){TY_INT_ARR, 8};
Type *ty_d_by = &(Type){TY_D_BY, 8};
Type *ty_enum = &(Type){TY_ENUM, 4};

bool is_integer(Type *ty) { return ty->kind == TY_INT; }

Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->base = base;
  return ty;
}

// Boolean formed intermediate within LR or.
bool lr_if_or(Node *nd, Kind k) {
  return nd->ty->kind == k || k == nd->rhs->ty->kind;
}

Type *type_arr(Type *ty) {
  if (ty == ty_char)
    return ty_char_arr;
  else if (ty == ty_int)
    return ty_int_arr;
  else
    error("array type expected but not got.");
  return NULL;
}

void typ_rev(Node *nd) {
  if (!nd || nd->ty)
    return;

  typ_rev(nd->lhs);
  typ_rev(nd->rhs);
  typ_rev(nd->cond);
  typ_rev(nd->stmt);

  typ_rev(nd->args);

  typ_rev(nd->els);
  typ_rev(nd->then);

  for (Node *n = nd->args; n; n = n->next)
    typ_rev(n);
  for (Node *n = nd->block; n; n = n->next)
    typ_rev(n);

  switch (nd->kind) {
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
    nd->ty = ty_int;
    return;
  case ND_PTR_ADD:
  case ND_PTR_SUB:
  case ND_ASSIGN:
    nd->ty = nd->lhs->ty;
    if (nd->lhs->v && nd->rhs->ty == ty_char)
      nd->lhs->v = nd->rhs->v;
    return;
  case ND_ADDR:
    nd->ty = pointer_to(nd->lhs->ty);
    return;
  case ND_REF:
    nd->ty = nd->lhs->ty;
    return;
  default:
    return;
  }
}
