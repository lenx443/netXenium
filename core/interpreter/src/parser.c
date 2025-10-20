#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "instance.h"
#include "lexer.h"
#include "parser.h"
#include "xen_ast.h"
#include "xen_nil.h"

static bool is_stmt(Parser*);
static bool is_expr(Parser*);
static bool is_primary(Parser*);
static bool is_unary(Parser*);
static bool is_factor(Parser*);
static bool is_assigment(Parser*);
static bool is_suffix(Parser*);
static bool is_keyword(Parser*);

static Xen_Instance* wrap_expr(Xen_Instance*);
static Xen_Instance* wrap_if_condition(Xen_Instance*);
static Xen_Instance* wrap_if_then(Xen_Instance*);
static Xen_Instance* wrap_if_else(Xen_Instance*);

static Xen_Instance* parser_stmt_list(Parser*);
static Xen_Instance* parser_stmt(Parser*);
static Xen_Instance* parser_string(Parser*);
static Xen_Instance* parser_number(Parser*);
static Xen_Instance* parser_literal(Parser*);
static Xen_Instance* parser_property(Parser*);
static Xen_Instance* parser_parent(Parser*);
static Xen_Instance* parser_expr(Parser*);
static Xen_Instance* parser_primary(Parser*);
static Xen_Instance* parser_unary(Parser*);
static Xen_Instance* parser_factor(Parser*);
static Xen_Instance* parser_term(Parser*);
static Xen_Instance* parser_add(Parser*);
static Xen_Instance* parser_relational(Parser*);
static Xen_Instance* parser_and(Parser*);
static Xen_Instance* parser_or(Parser*);
static Xen_Instance* parser_pair(Parser*);
static Xen_Instance* parser_list(Parser*);
static Xen_Instance* parser_suffix(Parser*);
static Xen_Instance* parser_assignment(Parser*);
static Xen_Instance* parser_call(Parser*);
static Xen_Instance* parser_arg_tail(Parser*);
static Xen_Instance* parser_index(Parser*);
static Xen_Instance* parser_attr(Parser*);
static Xen_Instance* parser_inc(Parser*);
static Xen_Instance* parser_dec(Parser*);
static Xen_Instance* parser_keyword(Parser*);
static Xen_Instance* parser_if_stmt(Parser*);
static Xen_Instance* parser_block(Parser*);

void parser_next(Parser* p) {
  p->token = lexer_next_token(p->lexer);
}

Xen_Instance* parser_program(Parser* p) {
  Xen_Instance* program = Xen_AST_Node_New("Program", NULL);
  if_nil_eval(program) {
    return nil;
  }
  Xen_Instance* stmt_list = parser_stmt_list(p);
  if_nil_eval(stmt_list) {
    Xen_DEL_REF(program);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(program, stmt_list)) {
    Xen_DEL_REF(stmt_list);
    Xen_DEL_REF(program);
    return nil;
  }
  Xen_DEL_REF(stmt_list);
  if (p->token.tkn_type != TKN_EOF) {
    Xen_DEL_REF(program);
    return nil;
  }
  return program;
}

bool is_stmt(Parser* p) {
  if (is_keyword(p)) {
    return true;
  }
  return false;
}

bool is_expr(Parser* p) {
  if (is_factor(p))
    return true;
  return false;
}

bool is_primary(Parser* p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_STRING || token == TKN_NUMBER || token == TKN_IDENTIFIER ||
      token == TKN_PROPERTY || token == TKN_LPARENT)
    return true;
  return false;
}

bool is_unary(Parser* p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_ADD || token == TKN_MINUS || token == TKN_NOT ||
      token == TKN_INC || token == TKN_DEC) {
    return true;
  }
  return false;
}

bool is_factor(Parser* p) {
  return is_unary(p) || is_primary(p);
}

bool is_assigment(Parser* p) {
  return is_factor(p);
}

bool is_suffix(Parser* p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_LPARENT || token == TKN_LBRACKET || token == TKN_ATTR ||
      token == TKN_INC || token == TKN_DEC)
    return true;
  return false;
}

bool is_keyword(Parser* p) {
  if (p->token.tkn_type == TKN_KEYWORD || is_assigment(p)) {
    return true;
  }
  return false;
}

Xen_Instance* wrap_expr(Xen_Instance* value) {
  Xen_Instance* expr = Xen_AST_Node_New("expr", NULL);
  if_nil_eval(expr) {
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(expr, value)) {
    Xen_DEL_REF(expr);
    return nil;
  }
  return expr;
}

Xen_Instance* wrap_if_condition(Xen_Instance* value) {
  Xen_Instance* condition = Xen_AST_Node_New("IfCondition", NULL);
  if_nil_eval(condition) {
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(condition, value)) {
    Xen_DEL_REF(condition);
    return nil;
  }
  return condition;
}

Xen_Instance* wrap_if_then(Xen_Instance* value) {
  Xen_Instance* then = Xen_AST_Node_New("IfThen", NULL);
  if_nil_eval(then) {
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(then, value)) {
    Xen_DEL_REF(then);
    return nil;
  }
  return then;
}

Xen_Instance* wrap_if_else(Xen_Instance* value) {
  Xen_Instance* els = Xen_AST_Node_New("IfElse", NULL);
  if_nil_eval(els) {
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(els, value)) {
    Xen_DEL_REF(els);
    return nil;
  }
  return els;
}

Xen_Instance* parser_stmt_list(Parser* p) {
  Xen_Instance* stmt_list = Xen_AST_Node_New("StatementList", NULL);
  if_nil_eval(stmt_list) {
    return nil;
  }
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  if (!is_stmt(p)) {
    return stmt_list;
  }
  Xen_Instance* stmt_head = parser_stmt(p);
  if_nil_eval(stmt_head) {
    Xen_DEL_REF(stmt_list);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(stmt_list, stmt_head)) {
    Xen_DEL_REF(stmt_head);
    Xen_DEL_REF(stmt_list);
    return nil;
  }
  Xen_DEL_REF(stmt_head);
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
    while (p->token.tkn_type == TKN_NEWLINE) {
      parser_next(p);
    }
    if (!is_stmt(p)) {
      return stmt_list;
    }
    Xen_Instance* stmt_tail = parser_stmt(p);
    if_nil_eval(stmt_tail) {
      Xen_DEL_REF(stmt_list);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(stmt_list, stmt_tail)) {
      Xen_DEL_REF(stmt_tail);
      Xen_DEL_REF(stmt_list);
      return nil;
    }
    Xen_DEL_REF(stmt_tail);
  }
  return stmt_list;
}

Xen_Instance* parser_stmt(Parser* p) {
  Xen_Instance* stmt = Xen_AST_Node_New("Statement", NULL);
  if_nil_eval(stmt) {
    return nil;
  }
  if (!is_stmt(p)) {
    return stmt;
  }
  Xen_Instance* stmt_val = parser_keyword(p);
  if_nil_eval(stmt_val) {
    Xen_DEL_REF(stmt);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(stmt, stmt_val)) {
    Xen_DEL_REF(stmt_val);
    Xen_DEL_REF(stmt);
    return nil;
  }
  Xen_DEL_REF(stmt_val);
  return stmt;
}

Xen_Instance* parser_string(Parser* p) {
  if (p->token.tkn_type != TKN_STRING)
    return nil;
  Xen_Instance* string = Xen_AST_Node_New("string", p->token.tkn_text);
  if_nil_eval(string) {
    return nil;
  }
  parser_next(p);
  return string;
}

Xen_Instance* parser_number(Parser* p) {
  if (p->token.tkn_type != TKN_NUMBER)
    return nil;
  Xen_Instance* number = Xen_AST_Node_New("number", p->token.tkn_text);
  if_nil_eval(number) {
    return nil;
  }
  parser_next(p);
  return number;
}

Xen_Instance* parser_literal(Parser* p) {
  if (p->token.tkn_type != TKN_IDENTIFIER)
    return nil;
  Xen_Instance* literal = Xen_AST_Node_New("literal", p->token.tkn_text);
  if_nil_eval(literal) {
    return nil;
  }
  parser_next(p);
  return literal;
}

Xen_Instance* parser_property(Parser* p) {
  if (p->token.tkn_type != TKN_PROPERTY)
    return nil;
  Xen_Instance* property = Xen_AST_Node_New("property", p->token.tkn_text);
  if_nil_eval(property) {
    return nil;
  }
  parser_next(p);
  return property;
}

Xen_Instance* parser_parent(Parser* p) {
  if (p->token.tkn_type != TKN_LPARENT)
    return nil;
  parser_next(p);
  Xen_Instance* parent = Xen_AST_Node_New("parent", NULL);
  if_nil_eval(parent) {
    return nil;
  }
  Xen_Instance* expr = parser_expr(p);
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

Xen_Instance* parser_expr(Parser* p) {
  Xen_Instance* value = parser_list(p);
  if_nil_eval(value) {
    return nil;
  }
  Xen_Instance* expr = wrap_expr(value);
  if_nil_eval(expr) {
    Xen_DEL_REF(value);
    return nil;
  }
  Xen_DEL_REF(value);
  return expr;
}

Xen_Instance* parser_primary(Parser* p) {
  Xen_Instance* value = nil;
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
  if_nil_eval(value) {
    return nil;
  }
  Xen_Instance* primary = Xen_AST_Node_New("primary", NULL);
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
    Xen_Instance* suffix = parser_suffix(p);
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

Xen_Instance* parser_unary(Parser* p) {
  if (!is_unary(p)) {
    return parser_primary(p);
  }
  Xen_Instance* unary = Xen_AST_Node_New("unary", p->token.tkn_text);
  if_nil_eval(unary) {
    return nil;
  }
  parser_next(p);
  Xen_Instance* primary = parser_primary(p);
  if_nil_eval(primary) {
    Xen_DEL_REF(unary);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(unary, primary)) {
    Xen_DEL_REF(primary);
    Xen_DEL_REF(unary);
    return nil;
  }
  Xen_DEL_REF(primary);
  return unary;
}

Xen_Instance* parser_factor(Parser* p) {
  if (is_factor(p)) {
    Xen_Instance* lhs = parser_unary(p);
    if_nil_eval(lhs) {
      return nil;
    }
    if (p->token.tkn_type == TKN_POW) {
      Xen_Instance* binary = Xen_AST_Node_New("binary", p->token.tkn_text);
      if_nil_eval(binary) {
        Xen_DEL_REF(lhs);
        return nil;
      }
      parser_next(p);
      Xen_Instance* rhs = parser_unary(p);
      if_nil_eval(rhs) {
        Xen_DEL_REF(lhs);
        Xen_DEL_REF(binary);
        return nil;
      }
      if (!Xen_AST_Node_Push_Child(binary, lhs)) {
        Xen_DEL_REF(rhs);
        Xen_DEL_REF(lhs);
        Xen_DEL_REF(binary);
        return nil;
      }
      if (!Xen_AST_Node_Push_Child(binary, rhs)) {
        Xen_DEL_REF(rhs);
        Xen_DEL_REF(lhs);
        Xen_DEL_REF(binary);
        return nil;
      }
      Xen_DEL_REF(rhs);
      Xen_DEL_REF(lhs);
      return binary;
    }
    return lhs;
  }
  return nil;
}

Xen_Instance* parser_term(Parser* p) {
  Xen_Instance* left = parser_factor(p);
  if_nil_eval(left) {
    return nil;
  }
  while (p->token.tkn_type == TKN_MUL || p->token.tkn_type == TKN_DIV ||
         p->token.tkn_type == TKN_MOD) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return nil;
    }
    parser_next(p);
    Xen_Instance* right = parser_factor(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_Instance* binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    free(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_add(Parser* p) {
  Xen_Instance* left = parser_term(p);
  if_nil_eval(left) {
    return nil;
  }
  while (p->token.tkn_type == TKN_ADD || p->token.tkn_type == TKN_MINUS) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return nil;
    }
    parser_next(p);
    Xen_Instance* right = parser_term(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_Instance* binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    free(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_relational(Parser* p) {
  Xen_Instance* left = parser_add(p);
  if_nil_eval(left) {
    return nil;
  }
  while (p->token.tkn_type == TKN_LT || p->token.tkn_type == TKN_GT ||
         p->token.tkn_type == TKN_LE || p->token.tkn_type == TKN_GE ||
         p->token.tkn_type == TKN_EQ || p->token.tkn_type == TKN_NE) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return nil;
    }
    parser_next(p);
    Xen_Instance* right = parser_add(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_Instance* binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    free(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_and(Parser* p) {
  Xen_Instance* left = parser_relational(p);
  if_nil_eval(left) {
    return nil;
  }
  while (p->token.tkn_type == TKN_AND) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return nil;
    }
    parser_next(p);
    Xen_Instance* right = parser_relational(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_Instance* binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    free(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_or(Parser* p) {
  Xen_Instance* left = parser_and(p);
  if_nil_eval(left) {
    return nil;
  }
  while (p->token.tkn_type == TKN_OR) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return nil;
    }
    parser_next(p);
    Xen_Instance* right = parser_and(p);
    if_nil_eval(right) {
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_Instance* binary = Xen_AST_Node_New("binary", op);
    if_nil_eval(binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return nil;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    free(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_pair(Parser* p) {
  Xen_Instance* first = parser_or(p);
  if_nil_eval(first) {
    return nil;
  }
  if (p->token.tkn_type != TKN_COLON) {
    return first;
  }
  parser_next(p);
  Xen_Instance* second = parser_or(p);
  if_nil_eval(second) {
    Xen_DEL_REF(first);
    return nil;
  }
  Xen_Instance* binary = Xen_AST_Node_New("binary", ":");
  if_nil_eval(binary) {
    Xen_DEL_REF(first);
    Xen_DEL_REF(second);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(binary, first)) {
    Xen_DEL_REF(first);
    Xen_DEL_REF(second);
    Xen_DEL_REF(binary);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(binary, second)) {
    Xen_DEL_REF(first);
    Xen_DEL_REF(second);
    Xen_DEL_REF(binary);
    return nil;
  }
  Xen_DEL_REF(first);
  Xen_DEL_REF(second);
  return binary;
}

Xen_Instance* parser_list(Parser* p) {
  Xen_Instance* expr_head = parser_pair(p);
  if_nil_eval(expr_head) {
    return nil;
  }
  if (p->token.tkn_type != TKN_COMMA) {
    return expr_head;
  }
  Xen_Instance* list = Xen_AST_Node_New("list", NULL);
  if_nil_eval(list) {
    Xen_DEL_REF(expr_head);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(list, expr_head)) {
    Xen_DEL_REF(expr_head);
    Xen_DEL_REF(list);
    return nil;
  }
  Xen_DEL_REF(expr_head);
  while (p->token.tkn_type == TKN_COMMA) {
    parser_next(p);
    if (!is_expr(p)) {
      return list;
    }
    Xen_Instance* expr = parser_pair(p);
    if_nil_eval(expr) {
      Xen_DEL_REF(list);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(list, expr)) {
      Xen_DEL_REF(expr);
      Xen_DEL_REF(list);
      return nil;
    }
    Xen_DEL_REF(expr);
  }
  return list;
}

Xen_Instance* parser_suffix(Parser* p) {
  Xen_Instance* suffix = Xen_AST_Node_New("suffix", NULL);
  if_nil_eval(suffix) {
    return nil;
  }
  if (p->token.tkn_type == TKN_LPARENT) {
    Xen_Instance* call = parser_call(p);
    if_nil_eval(call) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, call)) {
      Xen_DEL_REF(call);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(call);
  } else if (p->token.tkn_type == TKN_LBRACKET) {
    Xen_Instance* index = parser_index(p);
    if_nil_eval(index) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, index)) {
      Xen_DEL_REF(index);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(index);
  } else if (p->token.tkn_type == TKN_ATTR) {
    Xen_Instance* attr = parser_attr(p);
    if_nil_eval(attr) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, attr)) {
      Xen_DEL_REF(attr);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(attr);
  } else if (p->token.tkn_type == TKN_INC) {
    Xen_Instance* inc = parser_inc(p);
    if_nil_eval(inc) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, inc)) {
      Xen_DEL_REF(inc);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(inc);
  } else if (p->token.tkn_type == TKN_DEC) {
    Xen_Instance* dec = parser_dec(p);
    if_nil_eval(dec) {
      Xen_DEL_REF(suffix);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(suffix, dec)) {
      Xen_DEL_REF(dec);
      Xen_DEL_REF(suffix);
      return nil;
    }
    Xen_DEL_REF(dec);
  } else {
    Xen_DEL_REF(suffix);
    return nil;
  }
  if (is_suffix(p)) {
    Xen_Instance* next = parser_suffix(p);
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

Xen_Instance* parser_assignment(Parser* p) {
  Xen_Instance* lhs = parser_expr(p);
  if_nil_eval(lhs) {
    return nil;
  }

  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char* operator = strdup(p->token.tkn_text);
    parser_next(p);

    Xen_Instance* rhs = parser_expr(p);
    if_nil_eval(rhs) {
      Xen_DEL_REF(lhs);
      free((void*)operator);
      return nil;
    }

    Xen_Instance* assignm = Xen_AST_Node_New("assignment", operator);
    if_nil_eval(assignm) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      free((void*)operator);
      return nil;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      free((void*)operator);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      free((void*)operator);
      return nil;
    }
    Xen_DEL_REF(lhs);
    Xen_DEL_REF(rhs);
    free((void*)operator);
    return assignm;
  }
  return lhs;
}

Xen_Instance* parser_call(Parser* p) {
  if (p->token.tkn_type != TKN_LPARENT) {
    return nil;
  }
  parser_next(p);
  Xen_Instance* args = Xen_AST_Node_New("call", NULL);
  if_nil_eval(args) {
    return nil;
  }
  if (p->token.tkn_type == TKN_RPARENT) {
    parser_next(p);
    return args;
  }
  Xen_Instance* arg_expr = parser_pair(p);
  if_nil_eval(arg_expr) {
    Xen_DEL_REF(args);
    return nil;
  }
  Xen_Instance* arg_head = wrap_expr(arg_expr);
  if_nil_eval(arg_head) {
    Xen_DEL_REF(arg_expr);
    Xen_DEL_REF(args);
    return nil;
  }
  Xen_DEL_REF(arg_expr);
  if (!Xen_AST_Node_Push_Child(args, arg_head)) {
    Xen_DEL_REF(arg_head);
    Xen_DEL_REF(args);
    return nil;
  }
  Xen_DEL_REF(arg_head);
  while (p->token.tkn_type != TKN_RPARENT) {
    Xen_Instance* arg_tail = parser_arg_tail(p);
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

Xen_Instance* parser_arg_tail(Parser* p) {
  if (p->token.tkn_type != TKN_COMMA) {
    return nil;
  }
  parser_next(p);
  Xen_Instance* arg_expr = parser_pair(p);
  if_nil_eval(arg_expr) {
    return nil;
  }
  Xen_Instance* arg = wrap_expr(arg_expr);
  if_nil_eval(arg) {
    Xen_DEL_REF(arg_expr);
    return nil;
  }
  Xen_DEL_REF(arg_expr);
  return arg;
}

Xen_Instance* parser_index(Parser* p) {
  if (p->token.tkn_type != TKN_LBRACKET) {
    return nil;
  }
  parser_next(p);
  Xen_Instance* index = Xen_AST_Node_New("index", NULL);
  if_nil_eval(index) {
    return nil;
  }
  Xen_Instance* expr = parser_expr(p);
  if_nil_eval(expr) {
    Xen_DEL_REF(index);
    return nil;
  }
  if (p->token.tkn_type != TKN_RBRACKET) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(index);
    return nil;
  }
  parser_next(p);
  if (!Xen_AST_Node_Push_Child(index, expr)) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(index);
    return nil;
  }
  Xen_DEL_REF(expr);
  return index;
}

Xen_Instance* parser_attr(Parser* p) {
  if (p->token.tkn_type != TKN_ATTR) {
    return nil;
  }
  parser_next(p);
  if (p->token.tkn_type != TKN_IDENTIFIER) {
    return nil;
  }
  const char* ident = strdup(p->token.tkn_text);
  if (!ident) {
    return nil;
  }
  parser_next(p);
  Xen_Instance* attr = Xen_AST_Node_New("attr", ident);
  if_nil_eval(attr) {
    free((void*)ident);
    return nil;
  }
  free((void*)ident);
  return attr;
}

Xen_Instance* parser_inc(Parser* p) {
  if (p->token.tkn_type != TKN_INC) {
    return nil;
  }
  parser_next(p);
  return Xen_AST_Node_New("inc", NULL);
}

Xen_Instance* parser_dec(Parser* p) {
  if (p->token.tkn_type != TKN_DEC) {
    return nil;
  }
  parser_next(p);
  return Xen_AST_Node_New("dec", NULL);
}

Xen_Instance* parser_keyword(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    return parser_assignment(p);
  }
  if (strcmp(p->token.tkn_text, "if") == 0) {
    return parser_if_stmt(p);
  }
  return nil;
}

Xen_Instance* parser_if_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    return nil;
  }
  Xen_Instance* if_stmt = Xen_AST_Node_New("ifStatement", NULL);
  if_nil_eval(if_stmt) {
    return nil;
  }
  parser_next(p);
  Xen_Instance* condition_node = parser_expr(p);
  if_nil_eval(condition_node) {
    Xen_DEL_REF(if_stmt);
    return nil;
  }
  Xen_Instance* condition = wrap_if_condition(condition_node);
  if_nil_eval(condition) {
    Xen_DEL_REF(condition_node);
    Xen_DEL_REF(if_stmt);
    return nil;
  }
  Xen_DEL_REF(condition_node);
  Xen_Instance* then_node = parser_block(p);
  if_nil_eval(then_node) {
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return nil;
  }
  Xen_Instance* then = wrap_if_then(then_node);
  if_nil_eval(then) {
    Xen_DEL_REF(then_node);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return nil;
  }
  Xen_DEL_REF(then_node);
  if (!Xen_AST_Node_Push_Child(if_stmt, condition)) {
    Xen_DEL_REF(then);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return nil;
  }
  if (!Xen_AST_Node_Push_Child(if_stmt, then)) {
    Xen_DEL_REF(then);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return nil;
  }
  Xen_DEL_REF(then);
  Xen_DEL_REF(condition);
  while (p->token.tkn_type == TKN_KEYWORD &&
         strcmp(p->token.tkn_text, "elif") == 0) {
    parser_next(p);
    Xen_Instance* elif = Xen_AST_Node_New("IfElseIf", NULL);
    if_nil_eval(elif) {
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_Instance* elif_condition_node = parser_expr(p);
    if_nil_eval(elif_condition_node) {
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_Instance* elif_condition = wrap_if_condition(elif_condition_node);
    if_nil_eval(elif_condition) {
      Xen_DEL_REF(elif_condition_node);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_DEL_REF(elif_condition_node);
    Xen_Instance* elif_then_node = parser_block(p);
    if_nil_eval(elif_then_node) {
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_Instance* elif_then = wrap_if_then(elif_then_node);
    if_nil_eval(elif_then) {
      Xen_DEL_REF(elif_then_node);
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_DEL_REF(elif_then_node);
    if (!Xen_AST_Node_Push_Child(elif, elif_condition)) {
      Xen_DEL_REF(elif_then);
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(elif, elif_then)) {
      Xen_DEL_REF(elif_then);
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_DEL_REF(elif_condition);
    Xen_DEL_REF(elif_then);
    if (!Xen_AST_Node_Push_Child(if_stmt, elif)) {
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_DEL_REF(elif);
  }
  if (p->token.tkn_type == TKN_KEYWORD &&
      strcmp(p->token.tkn_text, "else") == 0) {
    parser_next(p);
    Xen_Instance* else_node = parser_block(p);
    if_nil_eval(else_node) {
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_Instance* els = wrap_if_else(else_node);
    if_nil_eval(els) {
      Xen_DEL_REF(else_node);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_DEL_REF(else_node);
    if (!Xen_AST_Node_Push_Child(if_stmt, els)) {
      Xen_DEL_REF(els);
      Xen_DEL_REF(if_stmt);
      return nil;
    }
    Xen_DEL_REF(els);
  }
  return if_stmt;
}

Xen_Instance* parser_block(Parser* p) {
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  if (p->token.tkn_type != TKN_BLOCK) {
    return nil;
  }
  parser_next(p);
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  Xen_Instance* block = Xen_AST_Node_New("block", NULL);
  if_nil_eval(block) {
    return nil;
  }
  if (p->token.tkn_type == TKN_LBRACE) {
    parser_next(p);
    Xen_Instance* stmt_list = parser_stmt_list(p);
    if_nil_eval(stmt_list) {
      Xen_DEL_REF(block);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(block, stmt_list)) {
      Xen_DEL_REF(stmt_list);
      Xen_DEL_REF(block);
      return nil;
    }
    Xen_DEL_REF(stmt_list);
    if (p->token.tkn_type != TKN_RBRACE) {
      Xen_DEL_REF(block);
      return nil;
    }
    parser_next(p);
  } else {
    Xen_Instance* stmt = parser_stmt(p);
    if_nil_eval(stmt) {
      Xen_DEL_REF(block);
      return nil;
    }
    if (!Xen_AST_Node_Push_Child(block, stmt)) {
      Xen_DEL_REF(stmt);
      Xen_DEL_REF(block);
      return nil;
    }
    Xen_DEL_REF(stmt);
  }
  return block;
}
