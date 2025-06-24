#ifndef __AST_H__
#define __AST_H__

typedef enum {
  ARG_LITERAL = 0,
  ARG_PROPERTY,
  ARG_CONCAT,
} ArgExprType;

struct ArgExpr_s {
  ArgExprType arg_type;
  union {
    const char *literal;
    const char *property;
    struct {
      struct ArgExpr_s **parts;
      int count;
    } concat;
  };
};

typedef struct ArgExpr_s ArgExpr_t;

typedef enum {
  AST_EMPTY = 0,
  AST_CMD,
} ASTNodeType;

struct AST_Node_s {
  ASTNodeType ast_type;
  union {
    struct {
      const char *cmd_name;
      ArgExpr_t **cmd_args;
      int arg_count;
    } cmd;
  };
};

typedef struct AST_Node_s AST_Node_t;

AST_Node_t *ast_make_empty();
AST_Node_t *ast_make_cmd(const char *, ArgExpr_t **, int);

ArgExpr_t *ast_make_arg_literal(const char *);
ArgExpr_t *ast_make_arg_property(const char *);
ArgExpr_t *ast_make_arg_concat(ArgExpr_t **, int count);

void ast_free(AST_Node_t *);
void ast_free_arg(ArgExpr_t *);

#endif
