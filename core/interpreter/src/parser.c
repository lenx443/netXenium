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
static bool is_suffix(Parser *);

static Xen_Instance *parser_string(Parser *);
static Xen_Instance *parser_literal(Parser *);
static Xen_Instance *parser_property(Parser *);
static Xen_Instance *parser_expr(Parser *);
static Xen_Instance *parser_primary(Parser *);
static Xen_Instance *parser_suffix(Parser *);
static Xen_Instance *parser_assignment(Parser *);
static Xen_Instance *parser_args(Parser *);

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

static Lexer_Token_Type parser_peek(Parser *p) {
  int start = p->lexer->pos;
  Lexer_Token_Type token = lexer_next_token(p->lexer).tkn_type;
  p->lexer->pos = start;
  return token;
}

Xen_Instance *parser_stmt(Parser *p) {
  if (parser_peek(p) == TKN_ASSIGNMENT) { return parser_assignment(p); }
  if (is_expr(p)) { return parser_expr(p); }
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    char *identifier_name = strdup(p->token.tkn_text);

    if (parser_peek(p) == TKN_ASSIGNMENT) {
      Xen_Instance *assignm = parser_assignment(p);
      if_nil_eval(assignm) {
        free(identifier_name);
        return nil;
      }
      free(identifier_name);
      return assignm;
    }

    parser_next(p);
    if (p->token.tkn_type == TKN_LPARENT) {
      parser_next(p);

      Xen_Instance *cmd = Xen_AST_Node_New("cmd", identifier_name);
      if_nil_eval(cmd) {
        free(identifier_name);
        return nil;
      }
      while (p->token.tkn_type != TKN_RPARENT) {
        Xen_Instance *arg = parser_expr(p);
        if_nil_eval(arg) {
          Xen_DEL_REF(cmd);
          free(identifier_name);
          return nil;
        }
        if (!Xen_AST_Node_Push_Child(cmd, arg)) {
          Xen_DEL_REF(cmd);
          Xen_DEL_REF(arg);
          free(identifier_name);
          return nil;
        }
        Xen_DEL_REF(arg);
      }

      free(identifier_name);
      parser_next(p);
      return cmd;
    }

    free(identifier_name);
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

static bool is_expr(Parser *p) {
  if (is_primary(p)) return true;
  return false;
}

static bool is_primary(Parser *p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_STRING || token == TKN_IDENTIFIER || token == TKN_PROPERTY)
    return true;
  return false;
}

static bool is_suffix(Parser *p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_LPARENT) return true;
  return false;
}

Xen_Instance *parser_string(Parser *p) {
  if (p->token.tkn_type != TKN_STRING) return 0;
  Xen_Instance *string = Xen_AST_Node_New("string", p->token.tkn_text);
  if_nil_eval(string) { return nil; }
  parser_next(p);
  return string;
}

Xen_Instance *parser_literal(Parser *p) {
  if (p->token.tkn_type != TKN_IDENTIFIER) return 0;
  Xen_Instance *literal = Xen_AST_Node_New("literal", p->token.tkn_text);
  if_nil_eval(literal) { return nil; }
  parser_next(p);
  return literal;
}

Xen_Instance *parser_property(Parser *p) {
  if (p->token.tkn_type != TKN_PROPERTY) return 0;
  Xen_Instance *property = Xen_AST_Node_New("property", p->token.tkn_text);
  if_nil_eval(property) { return nil; }
  parser_next(p);
  return property;
}

Xen_Instance *parser_expr(Parser *p) {
  Xen_Instance *expr = Xen_AST_Node_New("expr", NULL);
  if_nil_eval(expr) { return nil; }
  Xen_Instance *primary = parser_primary(p);
  if_nil_eval(primary) {
    Xen_DEL_REF(expr);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(expr, primary)) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(primary);
    return nil;
  }
  Xen_DEL_REF(primary);
  return expr;
}

Xen_Instance *parser_primary(Parser *p) {
  Xen_Instance *value = nil;
  if (p->token.tkn_type == TKN_STRING) {
    value = parser_string(p);
  } else if (p->token.tkn_type == TKN_IDENTIFIER) {
    value = parser_literal(p);
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    value = parser_property(p);
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
    if_nil_eval(suffix) { return nil; }
    if (!Xen_AST_Node_Push_Child(primary, suffix)) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(suffix);
  }
  return primary;
}

Xen_Instance *parser_suffix(Parser *p) {
  Xen_Instance *suffix = Xen_AST_Node_New("suffix", NULL);
  if_nil_eval(suffix) { return nil; }
  if (p->token.tkn_type == TKN_LPARENT) {
    Xen_Instance *args = parser_args(p);
    if_nil_eval(args) { return nil; }
    if (!Xen_AST_Node_Push_Child(suffix, args)) {
      Xen_DEL_REF(args);
      Xen_DEL_REF(suffix);
      return nil;
    }
  } else {
    Xen_DEL_REF(suffix);
    return nil;
  }
  return suffix;
}

Xen_Instance *parser_assignment(Parser *p) {
  Xen_Instance *lhs = Xen_AST_Node_New("literal", p->token.tkn_text); // LHS
  if_nil_eval(lhs) { return nil; }
  parser_next(p);

  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char *operator= "="; // por defecto
    parser_next(p);

    Xen_Instance *rhs = nil;
    if (p->token.tkn_type == TKN_STRING) {
      rhs = parser_string(p);
    } else if (p->token.tkn_type == TKN_IDENTIFIER) {
      rhs = parser_literal(p);
    } else if (p->token.tkn_type == TKN_PROPERTY) {
      rhs = parser_property(p);
    }
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

Xen_Instance *parser_args(Parser *p) { return nil; }
