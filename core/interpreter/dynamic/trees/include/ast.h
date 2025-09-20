#ifndef __AST_H__
#define __AST_H__

#include <stddef.h>

#include "operators.h"

typedef enum { ASSIGN_STRING } AssignmentExpession_RHS_Type;

typedef struct AssignmentExpession AssignmentExpession_t;

typedef enum {
  AST_EMPTY = 0,
  AST_STRING,
  AST_LITERAL,
  AST_PROPERTY,
  AST_ASSIGNMENT,
  AST_CMD,
} ASTNodeType;

struct AST_Node_s {
  ASTNodeType ast_type;
  union {
    struct {
      const char *value;
    } string;
    struct {
      const char *value;
    } literal;
    struct {
      const char *value;
    } property;
    struct {
      Xen_Opr operator;
      const char *lhs;
      struct AST_Node_s *rhs;
    } assignment;
    struct {
      const char *cmd_name;
      struct AST_Node_s **cmd_args;
      int arg_count;
    } cmd;
  };
};

typedef struct AST_Node_s AST_Node_t;

AST_Node_t *ast_make_empty();
AST_Node_t *ast_make_string(const char *);
AST_Node_t *ast_make_literal(const char *);
AST_Node_t *ast_make_property(const char *);
AST_Node_t *ast_make_assignment(const char *, AST_Node_t *);
AST_Node_t *ast_make_cmd(const char *, AST_Node_t **, int);

void ast_free(AST_Node_t *);
void ast_free_block(AST_Node_t **, size_t);

void ast_print(const AST_Node_t *node);
void ast_print_block(AST_Node_t **nodes, size_t count);

#endif
