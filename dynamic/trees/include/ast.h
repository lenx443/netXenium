typedef enum {
  ARG_LITERAL = 0,
  ARG_PROPERTY,
  ARG_CONCAT,
} ArgExprType;

typedef struct {
  ArgExprType arg_type;
  union {
    const char *literal;
    const char *property;
    struct {
      struct ArgExpr_t **parts;
      int count;
    } concat;
  };
} ArgExpr_t;

typedef enum {
  AST_CMD = 0,
} ASTNodeType;

typedef struct {
  ASTNodeType ast_type;
  union {
    struct {
      const char *cmd_name;
      ArgExpr_t **cmd_args;
      int arg_count;
    } cmd;
  };
} AST_Node_t;
