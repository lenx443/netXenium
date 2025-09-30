#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"
#include "xen_ast.h"
#include "xen_nil.h"

#define error(msg, ...) log_add(NULL, ERROR, "Parser", msg, ##__VA_ARGS__)

static bool is_expr(Parser *);
static bool is_primary(Parser *);
static bool is_factor(Parser *);
static bool is_suffix(Parser *);
static bool is_assigment(Parser *p);

static Xen_Instance *parser_string(Parser *);
static Xen_Instance *parser_number(Parser *);
static Xen_Instance *parser_literal(Parser *);
static Xen_Instance *parser_property(Parser *);
static Xen_Instance *parser_parent(Parser *);
static Xen_Instance *parser_expr(Parser *);
static Xen_Instance *parser_primary(Parser *);
static Xen_Instance *parser_factor(Parser *);
static Xen_Instance *parser_term(Parser *);
static Xen_Instance *parser_add(Parser *);
static Xen_Instance *parser_suffix(Parser *);
static Xen_Instance *parser_assignment(Parser *);
static Xen_Instance *parser_call(Parser *);
static Xen_Instance *parser_arg_tail(Parser *);

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

static Lexer_Token_Type parser_peek(Parser *p) {
  int start = p->lexer->pos;
  Lexer_Token_Type token = lexer_next_token(p->lexer).tkn_type;
  p->lexer->pos = start;
  return token;
}

Xen_Instance *parser_stmt(Parser *p) {
  if (is_assigment(p)) {
    return parser_assignment(p);
  } else if (is_expr(p)) {
    return parser_expr(p);
  } else if (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
    return parser_stmt(p);
  } else if (p->token.tkn_type == TKN_EOF) {
    return nil;
  }

  error("El token '%s' es invalido. Use `help` para mas información", p->token.tkn_text);
  parser_next(p);
  return Xen_AST_Node_New("empty", NULL);
}

bool is_expr(Parser *p) {
  if (is_primary(p)) return true;
  return false;
}

bool is_primary(Parser *p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_STRING || token == TKN_NUMBER || token == TKN_IDENTIFIER ||
      token == TKN_PROPERTY || token == TKN_LPARENT)
    return true;
  return false;
}

bool is_factor(Parser *p) { return is_primary(p); }

bool is_suffix(Parser *p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_LPARENT) return true;
  return false;
}

bool is_assigment(Parser *p) {
  if ((p->token.tkn_type == TKN_IDENTIFIER || p->token.tkn_type == TKN_PROPERTY) &&
      parser_peek(p) == TKN_ASSIGNMENT) {
    return true;
  }
  return false;
}

Xen_Instance *parser_string(Parser *p) {
  if (p->token.tkn_type != TKN_STRING) return nil;
  Xen_Instance *string = Xen_AST_Node_New("string", p->token.tkn_text);
  if_nil_eval(string) { return nil; }
  parser_next(p);
  return string;
}

Xen_Instance *parser_number(Parser *p) {
  if (p->token.tkn_type != TKN_NUMBER) return nil;
  Xen_Instance *number = Xen_AST_Node_New("number", p->token.tkn_text);
  if_nil_eval(number) { return nil; }
  parser_next(p);
  return number;
}

Xen_Instance *parser_literal(Parser *p) {
  if (p->token.tkn_type != TKN_IDENTIFIER) return nil;
  Xen_Instance *literal = Xen_AST_Node_New("literal", p->token.tkn_text);
  if_nil_eval(literal) { return nil; }
  parser_next(p);
  return literal;
}

Xen_Instance *parser_property(Parser *p) {
  if (p->token.tkn_type != TKN_PROPERTY) return nil;
  Xen_Instance *property = Xen_AST_Node_New("property", p->token.tkn_text);
  if_nil_eval(property) { return nil; }
  parser_next(p);
  return property;
}

Xen_Instance *parser_parent(Parser *p) {
  if (p->token.tkn_type != TKN_LPARENT) return nil;
  parser_next(p);
  Xen_Instance *parent = Xen_AST_Node_New("parent", NULL);
  if_nil_eval(parent) { return nil; }
  Xen_Instance *expr = parser_expr(p);
  if_nil_eval(expr) {
    Xen_DEL_REF(parent);
    return nil;
  }
  if (p->token.tkn_type != TKN_RPARENT) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(parent);
    return nil;
  }
  parser_next(p);
  if (!Xen_AST_Node_Push_Child(parent, expr)) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(parent);
    return nil;
  }
  Xen_DEL_REF(expr);
  return parent;
}

Xen_Instance *parser_expr(Parser *p) {
  Xen_Instance *expr = Xen_AST_Node_New("expr", NULL);
  if_nil_eval(expr) { return nil; }
  Xen_Instance *value = parser_add(p);
  if_nil_eval(value) {
    Xen_DEL_REF(expr);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(expr, value)) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(value);
    return nil;
  }
  Xen_DEL_REF(value);
  return expr;
}

Xen_Instance *parser_primary(Parser *p) {
  Xen_Instance *value = nil;
  if (p->token.tkn_type == TKN_STRING) {
    value = parser_string(p);
  } else if (p->token.tkn_type == TKN_NUMBER) {
    value = parser_number(p);
  } else if (p->token.tkn_type == TKN_IDENTIFIER) {
    value = parser_literal(p);
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    value = parser_property(p);
  } else if (p->token.tkn_type == TKN_LPARENT) {
    value = parser_parent(p);
  }
  if_nil_eval(value) { return nil; }
  Xen_Instance *primary = Xen_AST_Node_New("primary", NULL);
  if_nil_eval(primary) {
    Xen_DEL_REF(value);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(primary, value)) {
    Xen_DEL_REF(primary);
    Xen_DEL_REF(value);
    return nil;
  }
  Xen_DEL_REF(value);
  if (is_suffix(p)) {
    Xen_Instance *suffix = parser_suffix(p);
    if_nil_eval(suffix) {
      Xen_DEL_REF(primary);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(primary, suffix)) {
      Xen_DEL_REF(suffix);
      Xen_DEL_REF(primary);
      return nil;
    }
    Xen_DEL_REF(suffix);
  }
  return primary;
}

Xen_Instance *parser_factor(Parser *p) {
  if (is_factor(p)) { return parser_primary(p); }
  return nil;
}

Xen_Instance *parser_term(Parser *p) {
  Xen_Instance *left = parser_factor(p);
  if_nil_eval(left) { return nil; }
  while (p->token.tkn_type == TKN_MUL || p->token.tkn_type == TKN_DIV ||
         p->token.tkn_type == TKN_MOD) {
    char op[2];
    op[0] = p->token.tkn_text[0];
    op[1] = '\0';
    parser_next(p);
    Xen_Instance *right = parser_factor(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      return nil;
    }
    Xen_Instance *binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    left = binary;
  }
  return left;
}

Xen_Instance *parser_add(Parser *p) {
  Xen_Instance *left = parser_term(p);
  if_nil_eval(left) { return nil; }
  while (p->token.tkn_type == TKN_ADD || p->token.tkn_type == TKN_MINUS) {
    char op[2];
    op[0] = p->token.tkn_text[0];
    op[1] = '\0';
    parser_next(p);
    Xen_Instance *right = parser_term(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      return nil;
    }
    Xen_Instance *binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    left = binary;
  }
  return left;
}

Xen_Instance *parser_suffix(Parser *p) {
  Xen_Instance *suffix = Xen_AST_Node_New("suffix", NULL);
  if_nil_eval(suffix) { return nil; }
  if (p->token.tkn_type == TKN_LPARENT) {
    Xen_Instance *args = parser_call(p);
    if_nil_eval(args) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, args)) {
      Xen_DEL_REF(args);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(args);
  } else {
    Xen_DEL_REF(suffix);
    return nil;
  }
  if (is_suffix(p)) {
    Xen_Instance *next = parser_suffix(p);
    if_nil_eval(next) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, next)) {
      Xen_DEL_REF(next);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(next);
  }
  return suffix;
}

Xen_Instance *parser_assignment(Parser *p) {
  Xen_Instance *lhs = nil;
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    lhs = parser_literal(p);
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    lhs = parser_property(p);
  }
  if_nil_eval(lhs) { return nil; }

  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char *operator= "="; // por defecto
    parser_next(p);

    Xen_Instance *rhs = parser_expr(p);
    if_nil_eval(rhs) {
      Xen_DEL_REF(lhs);
      return nil;
    }

    Xen_Instance *assignm = Xen_AST_Node_New("assignment", operator);
    if_nil_eval(assignm) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      return nil;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      return nil;
    }
    Xen_DEL_REF(lhs);
    Xen_DEL_REF(rhs);

    parser_next(p);
    return assignm;
  }

  Xen_DEL_REF(lhs);
  return nil;
}

Xen_Instance *parser_call(Parser *p) {
  if (p->token.tkn_type != TKN_LPARENT) { return nil; }
  parser_next(p);
  Xen_Instance *args = Xen_AST_Node_New("call", NULL);
  if_nil_eval(args) { return nil; }
  if (p->token.tkn_type == TKN_RPARENT) {
    parser_next(p);
    return args;
  }
  Xen_Instance *arg_head = parser_expr(p);
  if_nil_eval(arg_head) {
    Xen_DEL_REF(args);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(args, arg_head)) {
    Xen_DEL_REF(arg_head);
    Xen_DEL_REF(args);
    return nil;
  }
  Xen_DEL_REF(arg_head);
  while (p->token.tkn_type != TKN_RPARENT) {
    Xen_Instance *arg_tail = parser_arg_tail(p);
    if_nil_eval(arg_tail) {
      Xen_DEL_REF(args);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(args, arg_tail)) {
      Xen_DEL_REF(arg_tail);
      Xen_DEL_REF(args);
      return nil;
    }
    Xen_DEL_REF(arg_tail);
  }
  parser_next(p);
  return args;
}

Xen_Instance *parser_arg_tail(Parser *p) {
  if (p->token.tkn_type != TKN_COMMA) { return nil; }
  parser_next(p);
  return parser_expr(p);
}
