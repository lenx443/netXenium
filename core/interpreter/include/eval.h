#ifndef __EVAL_H__
#define __EVAL_H__

#include "ast.h"

char *eval_arg(const ArgExpr_t *);
void ast_eval(const AST_Node_t *);

#endif
