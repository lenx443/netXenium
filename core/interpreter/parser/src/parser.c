#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "instance.h"
#include "lexer.h"
#include "parser.h"
#include "source_file.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_ast.h"
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_typedefs.h"

static void parser_next(Parser* p) {
  p->token = lexer_next_token(p->lexer);
}

static inline void skip_newline(Parser* p) {
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
}

static inline int skip_newline_if_before_is(Parser* p, Lexer_Token token) {
  int pos = p->lexer->pos;
  Lexer_Token tkn = p->token;
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  if (p->token.tkn_type != token.tkn_type ||
      strcmp(p->token.tkn_text, token.tkn_text) != 0) {
    p->lexer->pos = pos;
    p->token = tkn;
    return 0;
  }
  return 1;
}

static inline int skip_newline_if_callback(Parser* p,
                                           bool (*callback)(Parser*)) {
  int pos = p->lexer->pos;
  Lexer_Token tkn = p->token;
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  if (!callback(p)) {
    p->lexer->pos = pos;
    p->token = tkn;
    return 0;
  }
  return 1;
}

static bool is_stmt(Parser*);
static bool is_expr(Parser*);
static bool is_primary(Parser*);
static bool is_unary(Parser*);
static bool is_factor(Parser*);
static bool is_list(Parser*);
static bool is_assigment(Parser*);
static bool is_suffix(Parser*);
static bool is_keyword(Parser*);
static bool is_flow_keyword(Parser*);

static Xen_Instance* parser_stmt_list(Parser*);
static Xen_Instance* parser_stmt(Parser*);
static Xen_Instance* parser_string(Parser*);
static Xen_Instance* parser_number(Parser*);
static Xen_Instance* parser_nil(Parser*);
static Xen_Instance* parser_literal(Parser*);
static Xen_Instance* parser_property(Parser*);
static Xen_Instance* parser_parent(Parser*);
static Xen_Instance* parser_map(Parser*);
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
static Xen_Instance* parser_function(Parser*);
static Xen_Instance* parser_function_arg_tail(Parser*);
static Xen_Instance* parser_function_arg_assignment(Parser*);
static Xen_Instance* parser_list(Parser*);
static Xen_Instance* parser_suffix(Parser*);
static Xen_Instance* parser_assignment(Parser*);
static Xen_Instance* parser_call(Parser*);
static Xen_Instance* parser_arg_tail(Parser*);
static Xen_Instance* parser_arg_assignment(Parser*);
static Xen_Instance* parser_index(Parser*);
static Xen_Instance* parser_attr(Parser*);
static Xen_Instance* parser_keyword(Parser*);
static Xen_Instance* parser_if_stmt(Parser*);
static Xen_Instance* parser_while_stmt(Parser*);
static Xen_Instance* parser_for_stmt(Parser*);
static Xen_Instance* parser_block(Parser*);
static Xen_Instance* parser_flow_stmt(Parser*);
static Xen_Instance* parser_return_stmt(Parser*);
static Xen_Instance* parser_implement_stmt(Parser*);
static Xen_Instance* parser_throw_stmt(Parser*);
static Xen_Instance* parser_try_stmt(Parser*);

static Xen_Instance* parser_program(Parser* p) {
  Xen_Instance* program =
      Xen_AST_Node_New("Program", NULL, (Xen_Source_Address){0});
  if (!program) {
    return NULL;
  }
  Xen_Instance* stmt_list = parser_stmt_list(p);
  if (!stmt_list) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(program, stmt_list)) {
    return NULL;
  }
  if (p->token.tkn_type != TKN_EOF) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
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
  if (is_list(p))
    return true;
  return false;
}

bool is_primary(Parser* p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_STRING || token == TKN_NUMBER ||
      token == TKN_DOUBLE_QUESTION || token == TKN_IDENTIFIER ||
      token == TKN_PROPERTY || token == TKN_LPARENT || token == TKN_LBRACE)
    return true;
  return false;
}

bool is_unary(Parser* p) {
  Lexer_Token_Type token = p->token.tkn_type;
  if (token == TKN_ADD || token == TKN_MINUS || token == TKN_NOT ||
      token == TKN_MUL) {
    return true;
  }
  return false;
}

bool is_factor(Parser* p) {
  return is_unary(p) || is_primary(p);
}

bool is_list(Parser* p) {
  return p->token.tkn_type == TKN_QUESTION || p->token.tkn_type == TKN_BLOCK ||
         is_factor(p);
}

bool is_assigment(Parser* p) {
  return is_list(p);
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

bool is_flow_keyword(Parser* p) {
  Lexer_Token_Type type = p->token.tkn_type;
  Xen_string_t text = p->token.tkn_text;
  if (type != TKN_KEYWORD) {
    return 0;
  }
  if (strcmp(text, "break") == 0 || strcmp(text, "continue") == 0) {
    return 1;
  }
  return 0;
}

Xen_Instance* parser_stmt_list(Parser* p) {
  Xen_Instance* stmt_list =
      Xen_AST_Node_New("StatementList", NULL, p->token.sta);
  if (!stmt_list) {
    return NULL;
  }
  skip_newline(p);
  if (!is_stmt(p)) {
    return stmt_list;
  }
  Xen_Instance* stmt_head = parser_stmt(p);
  if (!stmt_head) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(stmt_list, stmt_head)) {
    return NULL;
  }
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
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(stmt_list, stmt_tail)) {
      return NULL;
    }
  }
  return stmt_list;
}

Xen_Instance* parser_stmt(Parser* p) {
  Xen_Instance* stmt = Xen_AST_Node_New("Statement", NULL, p->token.sta);
  if (!stmt) {
    return NULL;
  }
  if (!is_stmt(p)) {
    return stmt;
  }
  Xen_Instance* stmt_val = parser_keyword(p);
  if (!stmt_val) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(stmt, stmt_val)) {
    return NULL;
  }
  return stmt;
}

Xen_Instance* parser_string(Parser* p) {
  if (p->token.tkn_type != TKN_STRING) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* string =
      Xen_AST_Node_New("String", p->token.tkn_text, p->token.sta);
  if (!string) {
    return NULL;
  }
  parser_next(p);
  return string;
}

Xen_Instance* parser_number(Parser* p) {
  if (p->token.tkn_type != TKN_NUMBER) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* number =
      Xen_AST_Node_New("Number", p->token.tkn_text, p->token.sta);
  if (!number) {
    return NULL;
  }
  parser_next(p);
  return number;
}

Xen_Instance* parser_nil(Parser* p) {
  if (p->token.tkn_type != TKN_DOUBLE_QUESTION) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* expr_nil = Xen_AST_Node_New("Nil", NULL, p->token.sta);
  if (!expr_nil) {
    return NULL;
  }
  parser_next(p);
  return expr_nil;
}

Xen_Instance* parser_literal(Parser* p) {
  if (p->token.tkn_type != TKN_IDENTIFIER) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* literal =
      Xen_AST_Node_New("Literal", p->token.tkn_text, p->token.sta);
  if (!literal) {
    return NULL;
  }
  parser_next(p);
  return literal;
}

Xen_Instance* parser_property(Parser* p) {
  if (p->token.tkn_type != TKN_PROPERTY) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* property =
      Xen_AST_Node_New("Property", p->token.tkn_text, p->token.sta);
  if (!property) {
    return NULL;
  }
  parser_next(p);
  return property;
}

Xen_Instance* parser_parent(Parser* p) {
  if (p->token.tkn_type != TKN_LPARENT) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  Xen_Instance* parent = Xen_AST_Node_New("Parent", NULL, p->token.sta);
  if (!parent) {
    return NULL;
  }
  skip_newline(p);
  if (!is_expr(p)) {
    Xen_SyntaxError("Unclosed '('");
    return NULL;
  }
  Xen_Instance* expr = parser_expr(p);
  if (!expr) {
    return NULL;
  }
  skip_newline(p);
  if (p->token.tkn_type != TKN_RPARENT) {
    Xen_SyntaxError("Unclosed '('");
    return NULL;
  }
  parser_next(p);
  if (!Xen_AST_Node_Push_Child(parent, expr)) {
    return NULL;
  }
  return parent;
}

Xen_Instance* parser_map(Parser* p) {
  if (p->token.tkn_type != TKN_LBRACE) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* map = Xen_AST_Node_New("Map", NULL, p->token.sta);
  if (!map) {
    return NULL;
  }
  parser_next(p);
  skip_newline(p);
  if (p->token.tkn_type == TKN_RBRACE) {
    parser_next(p);
    return map;
  }
  Xen_Instance* element_head =
      Xen_AST_Node_New("MapElement", NULL, p->token.sta);
  if (!element_head) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LBRACKET) {
    parser_next(p);
    skip_newline(p);
    Xen_Instance* key = parser_primary(p);
    if (!key) {
      return NULL;
    }
    skip_newline(p);
    if (p->token.tkn_type != TKN_RBRACKET) {
      return NULL;
    }
    parser_next(p);
    if (!Xen_AST_Node_Push_Child(element_head, key)) {
      return NULL;
    }
  } else if (p->token.tkn_type == TKN_IDENTIFIER) {
    Xen_Instance* literal =
        Xen_AST_Node_New("Literal", p->token.tkn_text, p->token.sta);
    if (!literal) {
      return NULL;
    }
    parser_next(p);
    if (!Xen_AST_Node_Push_Child(element_head, literal)) {
      return NULL;
    }
  } else {
    Xen_SyntaxError("Invalid key in map literal");
    return NULL;
  }
  skip_newline(p);
  if (p->token.tkn_type == TKN_ARROW) {
    parser_next(p);
    skip_newline(p);
    Xen_Instance* expr_node = parser_function(p);
    if (!expr_node) {
      return NULL;
    }
    Xen_Instance* expr = Xen_AST_Node_Wrap(expr_node, "Expr");
    if (!expr) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(element_head, expr)) {
      return NULL;
    }
  }
  skip_newline(p);
  if (!Xen_AST_Node_Push_Child(map, element_head)) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_COMMA) {
    parser_next(p);
    skip_newline(p);
    Xen_Instance* element = Xen_AST_Node_New("MapElement", NULL, p->token.sta);
    if (!element) {
      return NULL;
    }
    if (p->token.tkn_type == TKN_LBRACKET) {
      parser_next(p);
      skip_newline(p);
      Xen_Instance* key = parser_primary(p);
      if (!key) {
        return NULL;
      }
      skip_newline(p);
      if (p->token.tkn_type != TKN_RBRACKET) {
        return NULL;
      }
      parser_next(p);
      if (!Xen_AST_Node_Push_Child(element, key)) {
        return NULL;
      }
    } else if (p->token.tkn_type == TKN_IDENTIFIER) {
      Xen_Instance* literal =
          Xen_AST_Node_New("Literal", p->token.tkn_text, p->token.sta);
      if (!literal) {
        return NULL;
      }
      parser_next(p);
      if (!Xen_AST_Node_Push_Child(element, literal)) {
        return NULL;
      }
    } else {
      Xen_SyntaxError("Invalid key in map literal");
      return NULL;
    }
    skip_newline(p);
    if (p->token.tkn_type == TKN_ARROW) {
      parser_next(p);
      skip_newline(p);
      Xen_Instance* expr_node = parser_function(p);
      if (!expr_node) {
        return NULL;
      }
      Xen_Instance* expr = Xen_AST_Node_Wrap(expr_node, "Expr");
      if (!expr) {
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(element, expr)) {
        return NULL;
      }
    }
    skip_newline(p);
    if (!Xen_AST_Node_Push_Child(map, element)) {
      return NULL;
    }
  }
  if (p->token.tkn_type != TKN_RBRACE) {
    Xen_SyntaxError("Unclosed '{'");
    return NULL;
  }
  parser_next(p);
  return map;
}

Xen_Instance* parser_expr(Parser* p) {
  Xen_Instance* value = parser_list(p);
  if (!value) {
    return NULL;
  }
  Xen_Instance* expr = Xen_AST_Node_Wrap(value, "Expr");
  if (!expr) {
    return NULL;
  }
  return expr;
}

Xen_Instance* parser_primary(Parser* p) {
  Xen_Instance* value = NULL;
  if (p->token.tkn_type == TKN_STRING) {
    value = parser_string(p);
  } else if (p->token.tkn_type == TKN_NUMBER) {
    value = parser_number(p);
  } else if (p->token.tkn_type == TKN_DOUBLE_QUESTION) {
    value = parser_nil(p);
  } else if (p->token.tkn_type == TKN_IDENTIFIER) {
    value = parser_literal(p);
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    value = parser_property(p);
  } else if (p->token.tkn_type == TKN_LPARENT) {
    value = parser_parent(p);
  } else if (p->token.tkn_type == TKN_LBRACE) {
    value = parser_map(p);
  } else {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  if (!value) {
    return NULL;
  }
  Xen_Instance* primary = Xen_AST_Node_New("Primary", NULL, p->token.sta);
  if (!primary) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(primary, value)) {
    return NULL;
  }
  if (is_suffix(p)) {
    Xen_Instance* suffix = parser_suffix(p);
    if (!suffix) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(primary, suffix)) {
      return NULL;
    }
  }
  return primary;
}

Xen_Instance* parser_unary(Parser* p) {
  if (!is_unary(p)) {
    return parser_primary(p);
  }
  Xen_Instance* unary =
      Xen_AST_Node_New("Unary", p->token.tkn_text, p->token.sta);
  if (!unary) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* primary = parser_primary(p);
  if (!primary) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(unary, primary)) {
    return NULL;
  }
  return unary;
}

Xen_Instance* parser_factor(Parser* p) {
  if (is_factor(p)) {
    Xen_Instance* lhs = parser_unary(p);
    if (!lhs) {
      return NULL;
    }
    if (p->token.tkn_type == TKN_POW) {
      Xen_Instance* binary =
          Xen_AST_Node_New("Binary", p->token.tkn_text, p->token.sta);
      if (!binary) {
        return NULL;
      }
      parser_next(p);
      Xen_Instance* rhs = parser_unary(p);
      if (!rhs) {
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(binary, lhs)) {
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(binary, rhs)) {
        return NULL;
      }
      return binary;
    }
    return lhs;
  }
  Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
  return NULL;
}

Xen_Instance* parser_term(Parser* p) {
  Xen_Instance* left = parser_factor(p);
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_MUL || p->token.tkn_type == TKN_DIV ||
         p->token.tkn_type == TKN_MOD) {
    char* op = Xen_CString_Dup(p->token.tkn_text);
    if (!op) {
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_factor(p);
    if (!right) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op, p->token.sta);
    if (!binary) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Dealloc(op);
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
    char* op = Xen_CString_Dup(p->token.tkn_text);
    if (!op) {
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_term(p);
    if (!right) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op, p->token.sta);
    if (!binary) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Dealloc(op);
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
         p->token.tkn_type == TKN_EQ || p->token.tkn_type == TKN_NE ||
         p->token.tkn_type == TKN_HAS) {
    char* op = Xen_CString_Dup(p->token.tkn_text);
    if (!op) {
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_add(p);
    if (!right) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op, p->token.sta);
    if (!binary) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Dealloc(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_not(Parser* p) {
  if (p->token.tkn_type != TKN_NOT) {
    return parser_relational(p);
  }
  Xen_Instance* unary =
      Xen_AST_Node_New("Unary", p->token.tkn_text, p->token.sta);
  if (!unary) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* relational = parser_not(p);
  if (!relational) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(unary, relational)) {
    return NULL;
  }
  return unary;
}

Xen_Instance* parser_and(Parser* p) {
  Xen_Instance* left = parser_not(p);
  if (!left) {
    return NULL;
  }
  while (p->token.tkn_type == TKN_AND) {
    char* op = Xen_CString_Dup(p->token.tkn_text);
    if (!op) {
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_not(p);
    if (!right) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op, p->token.sta);
    if (!binary) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Dealloc(op);
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
    char* op = Xen_CString_Dup(p->token.tkn_text);
    if (!op) {
      return NULL;
    }
    parser_next(p);
    Xen_Instance* right = parser_and(p);
    if (!right) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Instance* binary = Xen_AST_Node_New("Binary", op, p->token.sta);
    if (!binary) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, left)) {
      Xen_Dealloc(op);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(binary, right)) {
      Xen_Dealloc(op);
      return NULL;
    }
    Xen_Dealloc(op);
    left = binary;
  }
  return left;
}

Xen_Instance* parser_function(Parser* p) {
  if (p->token.tkn_type != TKN_BLOCK) {
    return parser_or(p);
  }
  parser_next(p);
  skip_newline(p);
  Xen_Instance* func = Xen_AST_Node_New("FunctionExpr", NULL, p->token.sta);
  if (!func) {
    return NULL;
  }
  Xen_Instance* args = Xen_AST_Node_New("Args", NULL, p->token.sta);
  if (!args) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LPARENT) {
    parser_next(p);
    skip_newline(p);
    if (p->token.tkn_type != TKN_RPARENT) {
      Xen_Instance* arg_head = parser_function_arg_assignment(p);
      if (!arg_head) {
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(args, arg_head)) {
        return NULL;
      }
      skip_newline(p);
      while (p->token.tkn_type != TKN_RPARENT) {
        Xen_Instance* arg_tail = parser_function_arg_tail(p);
        if (!arg_tail) {
          return NULL;
        }
        if (!Xen_AST_Node_Push_Child(args, arg_tail)) {
          return NULL;
        }
        skip_newline(p);
      }
    }
    parser_next(p);
    skip_newline(p);
  }
  if (!Xen_AST_Node_Push_Child(func, args)) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LBRACE) {
    parser_next(p);
    Xen_Instance* stmt_list = parser_stmt_list(p);
    if (!stmt_list) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(func, stmt_list)) {
      return NULL;
    }
    if (p->token.tkn_type != TKN_RBRACE) {
      Xen_SyntaxError("Unclosed '{'");
      return NULL;
    }
    parser_next(p);
  } else {
    Xen_Instance* stmt = parser_stmt(p);
    if (!stmt) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(func, stmt)) {
      return NULL;
    }
  }
  return func;
}

Xen_Instance* parser_function_arg_tail(Parser* p) {
  if (p->token.tkn_type != TKN_COMMA) {
    Xen_SyntaxError_Format("Expected COMMA but found '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  skip_newline(p);
  Xen_Instance* arg = parser_function_arg_assignment(p);
  if (!arg) {
    return NULL;
  }
  return arg;
}

Xen_Instance* parser_function_arg_assignment(Parser* p) {
  Xen_Instance* lhs_node = parser_or(p);
  if (!lhs_node) {
    return NULL;
  }
  Xen_Instance* lhs = Xen_AST_Node_Wrap(lhs_node, "Expr");
  if (!lhs) {
    return NULL;
  }
  skip_newline(p);
  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char* operator = Xen_CString_Dup(p->token.tkn_text);
    parser_next(p);
    skip_newline(p);
    Xen_Instance* rhs_node = parser_or(p);
    if (!rhs_node) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    Xen_Instance* rhs = Xen_AST_Node_Wrap(rhs_node, "Expr");
    if (!rhs) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }

    Xen_Instance* assignm =
        Xen_AST_Node_New("Assignment", operator, p->token.sta);
    if (!assignm) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    Xen_Dealloc((void*)operator);
    return assignm;
  } else if (p->token.tkn_type == TKN_BLOCK) {
    Xen_Instance* rhs = parser_function(p);
    if (!rhs) {
      return NULL;
    }

    Xen_Instance* assignm = Xen_AST_Node_New("Assignment", "=", p->token.sta);
    if (!assignm) {
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      return NULL;
    }
    return assignm;
  }
  return lhs;
}

Xen_Instance* parser_list(Parser* p) {
  int vector = 0;
  if (p->token.tkn_type == TKN_QUESTION) {
    vector = 1;
    parser_next(p);
    skip_newline_if_callback(p, is_expr);
  }
  Xen_Instance* expr_head = parser_function(p);
  if (!expr_head) {
    return NULL;
  }
  if (p->token.tkn_type != TKN_COMMA) {
    if (vector) {
      Xen_Instance* list = Xen_AST_Node_New("List", "vector", p->token.sta);
      if (!list) {
        return NULL;
      }
      if (!Xen_AST_Node_Push_Child(list, expr_head)) {
        return NULL;
      }
      return list;
    }
    return expr_head;
  }
  Xen_Instance* list =
      Xen_AST_Node_New("List", vector ? "vector" : "tuple", p->token.sta);
  if (!list) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(list, expr_head)) {
    return NULL;
  }
  skip_newline_if_before_is(p, (Lexer_Token){TKN_COMMA, ",", {0}});
  while (p->token.tkn_type == TKN_COMMA) {
    parser_next(p);
    skip_newline_if_callback(p, is_expr);
    if (!is_expr(p)) {
      return list;
    }
    Xen_Instance* expr = parser_function(p);
    if (!expr) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(list, expr)) {
      return NULL;
    }
  }
  return list;
}

Xen_Instance* parser_suffix(Parser* p) {
  Xen_Instance* suffix = Xen_AST_Node_New("Suffix", NULL, p->token.sta);
  if (!suffix) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LPARENT) {
    Xen_Instance* call = parser_call(p);
    if (!call) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, call)) {
      return NULL;
    }
  } else if (p->token.tkn_type == TKN_LBRACKET) {
    Xen_Instance* index = parser_index(p);
    if (!index) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, index)) {
      return NULL;
    }
  } else if (p->token.tkn_type == TKN_ATTR) {
    Xen_Instance* attr = parser_attr(p);
    if (!attr) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, attr)) {
      return NULL;
    }
  } else {
    Xen_SyntaxError("Invalid suffix");
    return NULL;
  }
  if (is_suffix(p)) {
    Xen_Instance* next = parser_suffix(p);
    if (!next) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(suffix, next)) {
      return NULL;
    }
  }
  return suffix;
}

Xen_Instance* parser_assignment(Parser* p) {
  Xen_Instance* lhs = parser_expr(p);
  if (!lhs) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char* operator = Xen_CString_Dup(p->token.tkn_text);
    parser_next(p);

    Xen_Instance* rhs = parser_expr(p);
    if (!rhs) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }

    Xen_Instance* assignm =
        Xen_AST_Node_New("Assignment", operator, p->token.sta);
    if (!assignm) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    Xen_Dealloc((void*)operator);
    return assignm;
  } else if (p->token.tkn_type == TKN_BLOCK) {
    Xen_Instance* rhs = parser_function(p);
    if (!rhs) {
      return NULL;
    }

    Xen_Instance* assignm = Xen_AST_Node_New("Assignment", "=", p->token.sta);
    if (!assignm) {
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      return NULL;
    }
    return assignm;
  }
  return lhs;
}

Xen_Instance* parser_call(Parser* p) {
  if (p->token.tkn_type != TKN_LPARENT) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  skip_newline(p);
  Xen_Instance* args = Xen_AST_Node_New("Call", NULL, p->token.sta);
  if (!args) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_RPARENT) {
    parser_next(p);
    return args;
  }
  Xen_Instance* arg_head = parser_arg_assignment(p);
  if (!arg_head) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(args, arg_head)) {
    return NULL;
  }
  skip_newline(p);
  while (p->token.tkn_type != TKN_RPARENT) {
    Xen_Instance* arg_tail = parser_arg_tail(p);
    if (!arg_tail) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(args, arg_tail)) {
      return NULL;
    }
    skip_newline(p);
  }
  parser_next(p);
  return args;
}

Xen_Instance* parser_arg_tail(Parser* p) {
  if (p->token.tkn_type != TKN_COMMA) {
    Xen_SyntaxError_Format("Expected COMMA but found '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  skip_newline(p);
  Xen_Instance* arg = parser_arg_assignment(p);
  if (!arg) {
    return NULL;
  }
  return arg;
}

Xen_Instance* parser_arg_assignment(Parser* p) {
  Xen_Instance* lhs_node = parser_function(p);
  if (!lhs_node) {
    return NULL;
  }
  Xen_Instance* lhs = Xen_AST_Node_Wrap(lhs_node, "Expr");
  if (!lhs) {
    return NULL;
  }
  skip_newline(p);
  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char* operator = Xen_CString_Dup(p->token.tkn_text);
    parser_next(p);
    skip_newline(p);

    Xen_Instance* rhs_node = parser_function(p);
    if (!rhs_node) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    Xen_Instance* rhs = Xen_AST_Node_Wrap(rhs_node, "Expr");
    if (!rhs) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }

    Xen_Instance* assignm =
        Xen_AST_Node_New("Assignment", operator, p->token.sta);
    if (!assignm) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      Xen_Dealloc((void*)operator);
      return NULL;
    }
    Xen_Dealloc((void*)operator);
    return assignm;
  } else if (p->token.tkn_type == TKN_BLOCK) {
    Xen_Instance* rhs = parser_function(p);
    if (!rhs) {
      return NULL;
    }

    Xen_Instance* assignm = Xen_AST_Node_New("Assignment", "=", p->token.sta);
    if (!assignm) {
      return NULL;
    }

    if (!Xen_AST_Node_Push_Child(assignm, lhs)) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(assignm, rhs)) {
      return NULL;
    }
    return assignm;
  }
  return lhs;
}

Xen_Instance* parser_index(Parser* p) {
  if (p->token.tkn_type != TKN_LBRACKET) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  Xen_Instance* index = Xen_AST_Node_New("Index", NULL, p->token.sta);
  if (!index) {
    return NULL;
  }
  Xen_Instance* expr = parser_expr(p);
  if (!expr) {
    return NULL;
  }
  if (p->token.tkn_type != TKN_RBRACKET) {
    Xen_SyntaxError("Unclosed '['");
    return NULL;
  }
  parser_next(p);
  if (!Xen_AST_Node_Push_Child(index, expr)) {
    return NULL;
  }
  return index;
}

Xen_Instance* parser_attr(Parser* p) {
  if (p->token.tkn_type != TKN_ATTR) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  if (p->token.tkn_type != TKN_IDENTIFIER) {
    Xen_SyntaxError("Identifier expected");
    return NULL;
  }
  const char* ident = Xen_CString_Dup(p->token.tkn_text);
  if (!ident) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* attr = Xen_AST_Node_New("Attr", ident, p->token.sta);
  if (!attr) {
    Xen_Dealloc((void*)ident);
    return NULL;
  }
  Xen_Dealloc((void*)ident);
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
  if (is_flow_keyword(p)) {
    return parser_flow_stmt(p);
  }
  if (strcmp(p->token.tkn_text, "return") == 0) {
    return parser_return_stmt(p);
  }
  if (strcmp(p->token.tkn_text, "implement") == 0) {
    return parser_implement_stmt(p);
  }
  if (strcmp(p->token.tkn_text, "throw") == 0) {
    return parser_throw_stmt(p);
  }
  if (strcmp(p->token.tkn_text, "try") == 0) {
    return parser_try_stmt(p);
  }
  Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
  return NULL;
}

Xen_Instance* parser_if_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* if_stmt = Xen_AST_Node_New("IfStatement", NULL, p->token.sta);
  if (!if_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* condition = parser_expr(p);
  if (!condition) {
    return NULL;
  }
  skip_newline(p);
  Xen_Instance* then = parser_block(p);
  if (!then) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(if_stmt, condition)) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(if_stmt, then)) {
    return NULL;
  }
  skip_newline_if_before_is(p, (Lexer_Token){TKN_KEYWORD, "elif", {0}});
  Xen_Instance* current_if = if_stmt;
  while (p->token.tkn_type == TKN_KEYWORD &&
         strcmp(p->token.tkn_text, "elif") == 0) {
    parser_next(p);
    Xen_Instance* elif = Xen_AST_Node_New("IfStatement", NULL, p->token.sta);
    if (!elif) {
      return NULL;
    }
    Xen_Instance* elif_condition = parser_expr(p);
    if (!elif_condition) {
      return NULL;
    }
    skip_newline(p);
    Xen_Instance* elif_then = parser_block(p);
    if (!elif_then) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(elif, elif_condition)) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(elif, elif_then)) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(current_if, elif)) {
      return NULL;
    }
    current_if = elif;
  }
  skip_newline_if_before_is(p, (Lexer_Token){TKN_KEYWORD, "else", {0}});
  if (p->token.tkn_type == TKN_KEYWORD &&
      strcmp(p->token.tkn_text, "else") == 0) {
    parser_next(p);
    skip_newline(p);
    Xen_Instance* els = parser_block(p);
    if (!els) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(current_if, els)) {
      return NULL;
    }
  }
  return if_stmt;
}

Xen_Instance* parser_while_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* while_stmt =
      Xen_AST_Node_New("WhileStatement", NULL, p->token.sta);
  if (!while_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* condition = parser_expr(p);
  if (!condition) {
    return NULL;
  }
  skip_newline(p);
  Xen_Instance* wdo = parser_block(p);
  if (!wdo) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(while_stmt, condition)) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(while_stmt, wdo)) {
    return NULL;
  }
  return while_stmt;
}

Xen_Instance* parser_for_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* for_stmt = Xen_AST_Node_New("ForStatement", NULL, p->token.sta);
  if (!for_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* target = parser_expr(p);
  if (!target) {
    return NULL;
  }
  if (p->token.tkn_type != TKN_KEYWORD ||
      strcmp(p->token.tkn_text, "in") != 0) {
    Xen_SyntaxError("Missing 'in' in for loop");
    return NULL;
  }
  parser_next(p);
  Xen_Instance* expr = parser_expr(p);
  if (!expr) {
    return NULL;
  }
  skip_newline(p);
  Xen_Instance* fdo = parser_block(p);
  if (!fdo) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(for_stmt, target)) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(for_stmt, expr)) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(for_stmt, fdo)) {
    return NULL;
  }
  return for_stmt;
}

Xen_Instance* parser_block(Parser* p) {
  if (p->token.tkn_type != TKN_BLOCK) {
    Xen_SyntaxError("Invalid block syntax");
    return NULL;
  }
  parser_next(p);
  skip_newline(p);
  Xen_Instance* block = NULL;
  block = Xen_AST_Node_New("Block", NULL, p->token.sta);
  if (!block) {
    return NULL;
  }
  if (p->token.tkn_type == TKN_LBRACE) {
    parser_next(p);
    Xen_Instance* stmt_list = parser_stmt_list(p);
    if (!stmt_list) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(block, stmt_list)) {
      return NULL;
    }
    if (p->token.tkn_type != TKN_RBRACE) {
      Xen_SyntaxError("Unclosed '{'");
      return NULL;
    }
    parser_next(p);
  } else {
    Xen_Instance* stmt = parser_stmt(p);
    if (!stmt) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(block, stmt)) {
      return NULL;
    }
  }
  return block;
}

Xen_Instance* parser_flow_stmt(Parser* p) {
  if (!is_flow_keyword(p)) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* flow =
      Xen_AST_Node_New("FlowStatement", p->token.tkn_text, p->token.sta);
  if (!flow) {
    return NULL;
  }
  parser_next(p);
  return flow;
}

Xen_Instance* parser_return_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* return_stmt =
      Xen_AST_Node_New("ReturnStatement", NULL, p->token.sta);
  if (!return_stmt) {
    return NULL;
  }
  parser_next(p);
  if (is_expr(p)) {
    Xen_Instance* expr = parser_expr(p);
    if (!expr) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(return_stmt, expr)) {
      return NULL;
    }
  }
  return return_stmt;
}

Xen_Instance* parser_implement_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  parser_next(p);
  if (p->token.tkn_type != TKN_IDENTIFIER) {
    Xen_SyntaxError("Invalid name for implementation");
    return NULL;
  }
  Xen_Instance* impl_stmt =
      Xen_AST_Node_New("ImplementStatement", p->token.tkn_text, p->token.sta);
  if (!impl_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* base = Xen_AST_Node_New("Base", NULL, p->token.sta);
  if (p->token.tkn_type == TKN_COLON) {
    parser_next(p);
    Xen_Instance* base_expr = parser_expr(p);
    if (!base_expr) {
      return NULL;
    }
    if (!Xen_AST_Node_Push_Child(base, base_expr)) {
      return NULL;
    }
  }
  if (!Xen_AST_Node_Push_Child(impl_stmt, base)) {
    return NULL;
  }
  skip_newline(p);
  Xen_Instance* impl_body = parser_block(p);
  if (!impl_body) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(impl_stmt, impl_body)) {
    return NULL;
  }
  return impl_stmt;
}

Xen_Instance* parser_throw_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* throw_stmt =
      Xen_AST_Node_New("ThrowStatement", NULL, p->token.sta);
  if (!throw_stmt) {
    return NULL;
  }
  parser_next(p);
  Xen_Instance* except = parser_expr(p);
  if (!except) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(throw_stmt, except)) {
    return NULL;
  }
  return throw_stmt;
}

Xen_Instance* parser_try_stmt(Parser* p) {
  if (p->token.tkn_type != TKN_KEYWORD) {
    Xen_SyntaxError_Format("Unexpected token '%s'", p->token.tkn_text);
    return NULL;
  }
  Xen_Instance* try_stmt = Xen_AST_Node_New("TryStatement", NULL, p->token.sta);
  if (!try_stmt) {
    return NULL;
  }
  parser_next(p);
  skip_newline(p);
  Xen_Instance* try_block = parser_block(p);
  if (!try_block) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(try_stmt, try_block)) {
    return NULL;
  }
  skip_newline(p);
  if (p->token.tkn_type != TKN_KEYWORD ||
      strcmp(p->token.tkn_text, "catch") != 0) {
    Xen_SyntaxError("'try' block requires a corresponding 'catch' block.");
    return NULL;
  }
  parser_next(p);
  Xen_Instance* catch_type = parser_string(p);
  if (!catch_type) {
    Xen_SyntaxError("Invalid exception type in 'catch' clause.");
    return NULL;
  }
  skip_newline(p);
  Xen_Instance* catch_block = parser_block(p);
  if (!catch_block) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(try_stmt, catch_block)) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(try_stmt, catch_type)) {
    return NULL;
  }
  return try_stmt;
}

Xen_Instance* Xen_Parser(Xen_c_string_t file_name, Xen_c_string_t file_content,
                         Xen_size_t file_size) {
  Xen_size_t sf_id = Xen_Source_Table_File_Push(
      globals_sources, Xen_Source_File_New(file_name, file_content, file_size));
  Lexer lexer = {sf_id, 1, 1, 0, 1, 1};
  Parser parser = {&lexer, {0, "\0", {0}}};
  parser_next(&parser);
  Xen_Instance* ast_program = parser_program(&parser);
  if (Xen_VM_Except_Active()) {
    Xen_VM_Except_Show(&parser.token.sta, 1);
    return NULL;
  }
  if (!ast_program) {
#ifndef NDEBUG
    Xen_Source_File* sf = globals_sources->st_files[sf_id];
    printf("Parser Error\n");
    printf("Current token: %d '%s'\n", parser.token.tkn_type,
           parser.token.tkn_text);
    printf("Pos: %ld\n", lexer.pos);
    Xen_ssize_t start =
        (Xen_ssize_t)(lexer.pos - 32) < 0 ? 0 : (lexer.pos - 32);
    Xen_size_t end =
        (lexer.pos + 32) > sf->sf_length ? sf->sf_length : (lexer.pos + 32);
    for (Xen_size_t i = (Xen_size_t)start; i < end; i++) {
      putchar(sf->sf_content[i]);
    }
    printf("\n");
#endif
    return NULL;
  }
  return ast_program;
}
