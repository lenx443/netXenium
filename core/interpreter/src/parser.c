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
static Xen_Instance* parser_not(Parser*);
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
static Xen_Instance* parser_keyword(Parser*);
static Xen_Instance* parser_if_stmt(Parser*);
static Xen_Instance* parser_while_stmt(Parser*);
static Xen_Instance* parser_for_stmt(Parser*);
static Xen_Instance* parser_block(Parser*);

void parser_next(Parser* p) {
  p->token = lexer_next_token(p->lexer);
}

Xen_Instance* parser_program(Parser* p) {
  Xen_Instance* program = Xen_AST_Node_New("Program", NULL);
  if (!program) {
    return NULL;
  }
  Xen_Instance* stmt_list = parser_stmt_list(p);
  if (!stmt_list) {
    Xen_DEL_REF(program);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(program, stmt_list)) {
    Xen_DEL_REF(stmt_list);
    Xen_DEL_REF(program);
    return NULL;
  }
  Xen_DEL_REF(stmt_list);
  if (p->token.tkn_type != TKN_EOF) {
    Xen_DEL_REF(program);
    return NULL;
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
  if (token == TKN_ADD || token == TKN_MINUS || token == TKN_NOT) {
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
  if (token == TKN_LPARENT || token == TKN_LBRACKET || token == TKN_ATTR)
    return true;
  return false;
}

bool is_keyword(Parser* p) {
  if (p->token.tkn_type == TKN_KEYWORD || is_assigment(p)) {
    return true;
  }
  return false;
}

Xen_Instance* parser_stmt_list(Parser* p) {
  Xen_Instance* stmt_list = Xen_AST_Node_New("StatementList", NULL);
  if (!stmt_list) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  if (!is_stmt(p)) {
    return stmt_list;
  }
  Xen_Instance* stmt_head = parser_stmt(p);
  if (!stmt_head) {
    Xen_DEL_REF(stmt_list);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(stmt_list, stmt_head)) {
    Xen_DEL_REF(stmt_head);
    Xen_DEL_REF(stmt_list);
    return NULL;
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
    if (!stmt_tail) {
      Xen_DEL_REF(stmt_list);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(stmt_list, stmt_tail)) {
      Xen_DEL_REF(stmt_tail);
      Xen_DEL_REF(stmt_list);
      return NULL;
    }
    Xen_DEL_REF(stmt_tail);
  }
  return stmt_list;
}

Xen_Instance* parser_stmt(Parser* p) {
  Xen_Instance* stmt = Xen_AST_Node_New("Statement", NULL);
  if (!stmt) {
    return NULL;
  }
  if (!is_stmt(p)) {
    return stmt;
  }
  Xen_Instance* stmt_val = parser_keyword(p);
  if (!stmt_val) {
    Xen_DEL_REF(stmt);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(stmt, stmt_val)) {
    Xen_DEL_REF(stmt_val);
    Xen_DEL_REF(stmt);
    return NULL;
  }
  Xen_DEL_REF(stmt_val);
  return stmt;
}

Xen_Instance* parser_string(Parser* p) {
  if (p->token.tkn_type != TKN_STRING)
    return NULL;
  Xen_Instance* string = Xen_AST_Node_New("String", p->token.tkn_text);
  if (!string) {
    return NULL;
  }
  parser_next(p);
  return string;
}

Xen_Instance* parser_number(Parser* p) {
  if (p->token.tkn_type != TKN_NUMBER)
    return NULL;
  Xen_Instance* number = Xen_AST_Node_New("Number", p->token.tkn_text);
  if (!number) {
    return NULL;
  }
  parser_next(p);
  return number;
}

Xen_Instance* parser_literal(Parser* p) {
  if (p->token.tkn_type != TKN_IDENTIFIER)
    return NULL;
  Xen_Instance* literal = Xen_AST_Node_New("Literal", p->token.tkn_text);
  if (!literal) {
    return NULL;
  }
  parser_next(p);
  return literal;
}

Xen_Instance* parser_property(Parser* p) {
  if (p->token.tkn_type != TKN_PROPERTY)
    return NULL;
  Xen_Instance* property = Xen_AST_Node_New("Property", p->token.tkn_text);
  if (!property) {
    return NULL;
  }
  parser_next(p);
  return property;
}

Xen_Instance* parser_parent(Parser* p) {
  if (p->token.tkn_type != TKN_LPARENT)
    return NULL;
  parser_next(p);
  Xen_Instance* parent = Xen_AST_Node_New("Parent", NULL);
  if (!parent) {
    return NULL;
  }
  Xen_Instance* expr = parser_expr(p);
  if (!expr) {
    Xen_DEL_REF(parent);
    return NULL;
  }
  if (p->token.tkn_type != TKN_RPARENT) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(parent);
    return NULL;
  }
  parser_next(p);
  if (!Xen_AST_Node_Push_Child(parent, expr)) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(parent);
    return NULL;
  }
  Xen_DEL_REF(expr);
  return parent;
}

Xen_Instance* parser_expr(Parser* p) {
  Xen_Instance* value = parser_list(p);
  if (!value) {
    return NULL;
  }
  Xen_Instance* expr = Xen_AST_Node_Wrap(value, "Expr");
  if (!expr) {
    Xen_DEL_REF(value);
    return NULL;
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
  if (!value) {
    return NULL;
  }
  Xen_Instance* primary = Xen_AST_Node_New("Primary", NULL);
  if (!primary) {
    Xen_DEL_REF(value);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(primary, value)) {
    Xen_DEL_REF(primary);
    Xen_DEL_REF(value);
    return NULL;
  }
  Xen_DEL_REF(value);
  if (is_suffix(p)) {
    Xen_Instance* suffix = parser_suffix(p);
    if (!suffix) {
      Xen_DEL_REF(primary);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(primary, suffix)) {
      Xen_DEL_REF(suffix);
      Xen_DEL_REF(primary);
      return NULL;
    }
    Xen_DEL_REF(suffix);
  }
  return primary;
}

Xen_Instance* parser_unary(Parser* p) {
  if (!is_unary(p)) {
    return parser_primary(p);
  }
  Xen_Instance* unary = Xen_AST_Node_New("Unary", p->token.tkn_text);
  if (!unary) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* primary = parser_primary(p);
  if (!primary) {
    Xen_DEL_REF(unary);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(unary, primary)) {
    Xen_DEL_REF(primary);
    Xen_DEL_REF(unary);
    return NULL;
  }
  Xen_DEL_REF(primary);
  return unary;
}

Xen_Instance* parser_factor(Parser* p) {
  if (is_factor(p)) {
    Xen_Instance* lhs = parser_unary(p);
    if (!lhs) {
      return NULL;
    }
    if (p->token.tkn_type == TKN_POW) {
      Xen_Instance* binary = Xen_AST_Node_New("Binary", p->token.tkn_text);
      if (!binary) {
        Xen_DEL_REF(lhs);
        return NULL;
      }
      parser_next(p);
      Xen_Instance* rhs = parser_unary(p);
      if (!rhs) {
        Xen_DEL_REF(lhs);
        Xen_DEL_REF(binary);
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(binary, lhs)) {
        Xen_DEL_REF(rhs);
        Xen_DEL_REF(lhs);
        Xen_DEL_REF(binary);
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(binary, rhs)) {
        Xen_DEL_REF(rhs);
        Xen_DEL_REF(lhs);
        Xen_DEL_REF(binary);
        return NULL;
      }
      Xen_DEL_REF(rhs);
      Xen_DEL_REF(lhs);
      return binary;
    }
    return lhs;
  }
  return NULL;
}

Xen_Instance* parser_term(Parser* p) {
  Xen_Instance* left = parser_factor(p);
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_MUL || p->token.tkn_type == TKN_DIV ||
         p->token.tkn_type == TKN_MOD) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_factor(p);
    if (!right) {
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op);
    if (!binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
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
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_ADD || p->token.tkn_type == TKN_MINUS) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_term(p);
    if (!right) {
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op);
    if (!binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
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
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_LT || p->token.tkn_type == TKN_GT ||
         p->token.tkn_type == TKN_LE || p->token.tkn_type == TKN_GE ||
         p->token.tkn_type == TKN_EQ || p->token.tkn_type == TKN_NE) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_add(p);
    if (!right) {
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op);
    if (!binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    Xen_DEL_REF(right);
    Xen_DEL_REF(left);
    free(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_not(Parser* p) {
  if (p->token.tkn_type != TKN_NOT) {
    return parser_relational(p);
  }
  Xen_Instance* unary = Xen_AST_Node_New("Unary", p->token.tkn_text);
  if (!unary) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* relational = parser_not(p);
  if (!relational) {
    Xen_DEL_REF(unary);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(unary, relational)) {
    Xen_DEL_REF(relational);
    Xen_DEL_REF(unary);
    return NULL;
  }
  Xen_DEL_REF(relational);
  return unary;
}

Xen_Instance* parser_and(Parser* p) {
  Xen_Instance* left = parser_not(p);
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_AND) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_not(p);
    if (!right) {
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op);
    if (!binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
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
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_OR) {
    char* op = strdup(p->token.tkn_text);
    if (!op) {
      Xen_DEL_REF(left);
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_and(p);
    if (!right) {
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op);
    if (!binary) {
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_DEL_REF(binary);
      Xen_DEL_REF(right);
      Xen_DEL_REF(left);
      free(op);
      return NULL;
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
  if (!first) {
    return NULL;
  }
  if (p->token.tkn_type != TKN_COLON) {
    return first;
  }
  parser_next(p);
  Xen_Instance* second = parser_or(p);
  if (!second) {
    Xen_DEL_REF(first);
    return NULL;
  }
  Xen_Instance* binary = Xen_AST_Node_New("Binary", ":");
  if (!binary) {
    Xen_DEL_REF(first);
    Xen_DEL_REF(second);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(binary, first)) {
    Xen_DEL_REF(first);
    Xen_DEL_REF(second);
    Xen_DEL_REF(binary);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(binary, second)) {
    Xen_DEL_REF(first);
    Xen_DEL_REF(second);
    Xen_DEL_REF(binary);
    return NULL;
  }
  Xen_DEL_REF(first);
  Xen_DEL_REF(second);
  return binary;
}

Xen_Instance* parser_list(Parser* p) {
  Xen_Instance* expr_head = parser_pair(p);
  if (!expr_head) {
    return NULL;
  }
  if (p->token.tkn_type != TKN_COMMA) {
    return expr_head;
  }
  Xen_Instance* list = Xen_AST_Node_New("List", NULL);
  if (!list) {
    Xen_DEL_REF(expr_head);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(list, expr_head)) {
    Xen_DEL_REF(expr_head);
    Xen_DEL_REF(list);
    return NULL;
  }
  Xen_DEL_REF(expr_head);
  while (p->token.tkn_type == TKN_COMMA) {
    parser_next(p);
    if (!is_expr(p)) {
      return list;
    }
    Xen_Instance* expr = parser_pair(p);
    if (!expr) {
      Xen_DEL_REF(list);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(list, expr)) {
      Xen_DEL_REF(expr);
      Xen_DEL_REF(list);
      return NULL;
    }
    Xen_DEL_REF(expr);
  }
  return list;
}

Xen_Instance* parser_suffix(Parser* p) {
  Xen_Instance* suffix = Xen_AST_Node_New("Suffix", NULL);
  if (!suffix) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LPARENT) {
    Xen_Instance* call = parser_call(p);
    if (!call) {
      Xen_DEL_REF(suffix);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, call)) {
      Xen_DEL_REF(call);
      Xen_DEL_REF(suffix);
      return NULL;
    }
    Xen_DEL_REF(call);
  } else if (p->token.tkn_type == TKN_LBRACKET) {
    Xen_Instance* index = parser_index(p);
    if (!index) {
      Xen_DEL_REF(suffix);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, index)) {
      Xen_DEL_REF(index);
      Xen_DEL_REF(suffix);
      return NULL;
    }
    Xen_DEL_REF(index);
  } else if (p->token.tkn_type == TKN_ATTR) {
    Xen_Instance* attr = parser_attr(p);
    if (!attr) {
      Xen_DEL_REF(suffix);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, attr)) {
      Xen_DEL_REF(attr);
      Xen_DEL_REF(suffix);
      return NULL;
    }
    Xen_DEL_REF(attr);
  } else {
    Xen_DEL_REF(suffix);
    return NULL;
  }
  if (is_suffix(p)) {
    Xen_Instance* next = parser_suffix(p);
    if (!next) {
      Xen_DEL_REF(suffix);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, next)) {
      Xen_DEL_REF(next);
      Xen_DEL_REF(suffix);
      return NULL;
    }
    Xen_DEL_REF(next);
  }
  return suffix;
}

Xen_Instance* parser_assignment(Parser* p) {
  Xen_Instance* lhs = parser_expr(p);
  if (!lhs) {
    return NULL;
  }

  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char* operator = strdup(p->token.tkn_text);
    parser_next(p);

    Xen_Instance* rhs = parser_expr(p);
    if (!rhs) {
      Xen_DEL_REF(lhs);
      free((void*)operator);
      return NULL;
    }

    Xen_Instance* assignm = Xen_AST_Node_New("Assignment", operator);
    if (!assignm) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      free((void*)operator);
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      free((void*)operator);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      Xen_DEL_REF(lhs);
      Xen_DEL_REF(rhs);
      free((void*)operator);
      return NULL;
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
    return NULL;
  }
  parser_next(p);
  Xen_Instance* args = Xen_AST_Node_New("Call", NULL);
  if (!args) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_RPARENT) {
    parser_next(p);
    return args;
  }
  Xen_Instance* arg_expr = parser_pair(p);
  if (!arg_expr) {
    Xen_DEL_REF(args);
    return NULL;
  }
  Xen_Instance* arg_head = Xen_AST_Node_Wrap(arg_expr, "Expr");
  if (!arg_head) {
    Xen_DEL_REF(arg_expr);
    Xen_DEL_REF(args);
    return NULL;
  }
  Xen_DEL_REF(arg_expr);
  if (!Xen_AST_Node_Push_Child(args, arg_head)) {
    Xen_DEL_REF(arg_head);
    Xen_DEL_REF(args);
    return NULL;
  }
  Xen_DEL_REF(arg_head);
  while (p->token.tkn_type != TKN_RPARENT) {
    Xen_Instance* arg_tail = parser_arg_tail(p);
    if (!arg_tail) {
      Xen_DEL_REF(args);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(args, arg_tail)) {
      Xen_DEL_REF(arg_tail);
      Xen_DEL_REF(args);
      return NULL;
    }
    Xen_DEL_REF(arg_tail);
  }
  parser_next(p);
  return args;
}

Xen_Instance* parser_arg_tail(Parser* p) {
  if (p->token.tkn_type != TKN_COMMA) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* arg_expr = parser_pair(p);
  if (!arg_expr) {
    return NULL;
  }
  Xen_Instance* arg = Xen_AST_Node_Wrap(arg_expr, "Expr");
  if (!arg) {
    Xen_DEL_REF(arg_expr);
    return NULL;
  }
  Xen_DEL_REF(arg_expr);
  return arg;
}

Xen_Instance* parser_index(Parser* p) {
  if (p->token.tkn_type != TKN_LBRACKET) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* index = Xen_AST_Node_New("Index", NULL);
  if (!index) {
    return NULL;
  }
  Xen_Instance* expr = parser_expr(p);
  if (!expr) {
    Xen_DEL_REF(index);
    return NULL;
  }
  if (p->token.tkn_type != TKN_RBRACKET) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(index);
    return NULL;
  }
  parser_next(p);
  if (!Xen_AST_Node_Push_Child(index, expr)) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(index);
    return NULL;
  }
  Xen_DEL_REF(expr);
  return index;
}

Xen_Instance* parser_attr(Parser* p) {
  if (p->token.tkn_type != TKN_ATTR) {
    return NULL;
  }
  parser_next(p);
  if (p->token.tkn_type != TKN_IDENTIFIER) {
    return NULL;
  }
  const char* ident = strdup(p->token.tkn_text);
  if (!ident) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* attr = Xen_AST_Node_New("Attr", ident);
  if (!attr) {
    free((void*)ident);
    return NULL;
  }
  free((void*)ident);
  return attr;
}

Xen_Instance* parser_keyword(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    return parser_assignment(p);
  }
  if (strcmp(p->token.tkn_text, "if") == 0) {
    return parser_if_stmt(p);
  }
  if (strcmp(p->token.tkn_text, "while") == 0) {
    return parser_while_stmt(p);
  }
  if (strcmp(p->token.tkn_text, "for") == 0) {
    return parser_for_stmt(p);
  }
  return NULL;
}

Xen_Instance* parser_if_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    return NULL;
  }
  Xen_Instance* if_stmt = Xen_AST_Node_New("IfStatement", NULL);
  if (!if_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* condition_node = parser_expr(p);
  if (!condition_node) {
    Xen_DEL_REF(if_stmt);
    return NULL;
  }
  Xen_Instance* condition = Xen_AST_Node_Wrap(condition_node, "IfCondition");
  if (!condition) {
    Xen_DEL_REF(condition_node);
    Xen_DEL_REF(if_stmt);
    return NULL;
  }
  Xen_DEL_REF(condition_node);
  Xen_Instance* then_node = parser_block(p);
  if (!then_node) {
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return NULL;
  }
  Xen_Instance* then = Xen_AST_Node_Wrap(then_node, "IfThen");
  if (!then) {
    Xen_DEL_REF(then_node);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return NULL;
  }
  Xen_DEL_REF(then_node);
  if (!Xen_AST_Node_Push_Child(if_stmt, condition)) {
    Xen_DEL_REF(then);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(if_stmt, then)) {
    Xen_DEL_REF(then);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(if_stmt);
    return NULL;
  }
  Xen_DEL_REF(then);
  Xen_DEL_REF(condition);
  while (p->token.tkn_type == TKN_KEYWORD &&
         strcmp(p->token.tkn_text, "elif") == 0) {
    parser_next(p);
    Xen_Instance* elif = Xen_AST_Node_New("IfElseIf", NULL);
    if (!elif) {
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_Instance* elif_condition_node = parser_expr(p);
    if (!elif_condition_node) {
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_Instance* elif_condition =
        Xen_AST_Node_Wrap(elif_condition_node, "IfCondition");
    if (!elif_condition) {
      Xen_DEL_REF(elif_condition_node);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_DEL_REF(elif_condition_node);
    Xen_Instance* elif_then_node = parser_block(p);
    if (!elif_then_node) {
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_Instance* elif_then = Xen_AST_Node_Wrap(elif_then_node, "IfThen");
    if (!elif_then) {
      Xen_DEL_REF(elif_then_node);
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_DEL_REF(elif_then_node);
    if (!Xen_AST_Node_Push_Child(elif, elif_condition)) {
      Xen_DEL_REF(elif_then);
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(elif, elif_then)) {
      Xen_DEL_REF(elif_then);
      Xen_DEL_REF(elif_condition);
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_DEL_REF(elif_condition);
    Xen_DEL_REF(elif_then);
    if (!Xen_AST_Node_Push_Child(if_stmt, elif)) {
      Xen_DEL_REF(elif);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_DEL_REF(elif);
  }
  if (p->token.tkn_type == TKN_KEYWORD &&
      strcmp(p->token.tkn_text, "else") == 0) {
    parser_next(p);
    Xen_Instance* else_node = parser_block(p);
    if (!else_node) {
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_Instance* els = Xen_AST_Node_Wrap(else_node, "IfElse");
    if (!els) {
      Xen_DEL_REF(else_node);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_DEL_REF(else_node);
    if (!Xen_AST_Node_Push_Child(if_stmt, els)) {
      Xen_DEL_REF(els);
      Xen_DEL_REF(if_stmt);
      return NULL;
    }
    Xen_DEL_REF(els);
  }
  return if_stmt;
}

Xen_Instance* parser_while_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    return NULL;
  }
  Xen_Instance* while_stmt = Xen_AST_Node_New("WhileStatement", NULL);
  if (!while_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* condition_node = parser_expr(p);
  if (!condition_node) {
    Xen_DEL_REF(while_stmt);
    return NULL;
  }
  Xen_Instance* condition = Xen_AST_Node_Wrap(condition_node, "WhileCondition");
  if (!condition) {
    Xen_DEL_REF(condition_node);
    Xen_DEL_REF(while_stmt);
    return NULL;
  }
  Xen_DEL_REF(condition_node);
  Xen_Instance* do_node = parser_block(p);
  if (!do_node) {
    Xen_DEL_REF(condition);
    Xen_DEL_REF(while_stmt);
    return NULL;
  }
  Xen_Instance* wdo = Xen_AST_Node_Wrap(do_node, "WhileDo");
  if (!wdo) {
    Xen_DEL_REF(do_node);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(while_stmt);
    return NULL;
  }
  Xen_DEL_REF(do_node);
  if (!Xen_AST_Node_Push_Child(while_stmt, condition)) {
    Xen_DEL_REF(wdo);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(while_stmt);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(while_stmt, wdo)) {
    Xen_DEL_REF(wdo);
    Xen_DEL_REF(condition);
    Xen_DEL_REF(while_stmt);
    return NULL;
  }
  Xen_DEL_REF(wdo);
  Xen_DEL_REF(condition);
  return while_stmt;
}

Xen_Instance* parser_for_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    return NULL;
  }
  Xen_Instance* for_stmt = Xen_AST_Node_New("ForStatement", NULL);
  if (!for_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* target_node = parser_expr(p);
  if (!target_node) {
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_Instance* target = Xen_AST_Node_Wrap(target_node, "ForTarget");
  if (!target) {
    Xen_DEL_REF(target_node);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_DEL_REF(target_node);
  if (p->token.tkn_type != TKN_KEYWORD ||
      strcmp(p->token.tkn_text, "in") != 0) {
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  parser_next(p);
  Xen_Instance* expr_node = parser_expr(p);
  if (!expr_node) {
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_Instance* expr = Xen_AST_Node_Wrap(expr_node, "ForExpr");
  if (!expr) {
    Xen_DEL_REF(expr_node);
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_DEL_REF(expr_node);
  Xen_Instance* do_node = parser_block(p);
  if (!do_node) {
    Xen_DEL_REF(expr);
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_Instance* fdo = Xen_AST_Node_Wrap(do_node, "ForDo");
  if (!fdo) {
    Xen_DEL_REF(do_node);
    Xen_DEL_REF(expr);
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_DEL_REF(do_node);
  if (!Xen_AST_Node_Push_Child(for_stmt, target)) {
    Xen_DEL_REF(fdo);
    Xen_DEL_REF(expr);
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(for_stmt, expr)) {
    Xen_DEL_REF(fdo);
    Xen_DEL_REF(expr);
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(for_stmt, fdo)) {
    Xen_DEL_REF(fdo);
    Xen_DEL_REF(expr);
    Xen_DEL_REF(target);
    Xen_DEL_REF(for_stmt);
    return NULL;
  }
  Xen_DEL_REF(fdo);
  Xen_DEL_REF(expr);
  Xen_DEL_REF(target);
  return for_stmt;
}

Xen_Instance* parser_block(Parser* p) {
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  if (p->token.tkn_type != TKN_BLOCK) {
    return NULL;
  }
  parser_next(p);
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  Xen_Instance* block = Xen_AST_Node_New("Block", NULL);
  if (!block) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LBRACE) {
    parser_next(p);
    Xen_Instance* stmt_list = parser_stmt_list(p);
    if (!stmt_list) {
      Xen_DEL_REF(block);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(block, stmt_list)) {
      Xen_DEL_REF(stmt_list);
      Xen_DEL_REF(block);
      return NULL;
    }
    Xen_DEL_REF(stmt_list);
    if (p->token.tkn_type != TKN_RBRACE) {
      Xen_DEL_REF(block);
      return NULL;
    }
    parser_next(p);
  } else {
    Xen_Instance* stmt = parser_stmt(p);
    if (!stmt) {
      Xen_DEL_REF(block);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(block, stmt)) {
      Xen_DEL_REF(stmt);
      Xen_DEL_REF(block);
      return NULL;
    }
    Xen_DEL_REF(stmt);
  }
  return block;
}
