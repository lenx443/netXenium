#include "instance.h"
#include "operators.h"
#include "xen_ast.h"
#include "xen_ast_implement.h"
#include "xen_ast_instance.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_vector.h"
#include <string.h>

Xen_Instance *Xen_AST_Node_New(const char *name, const char *value) {
  Xen_AST_Node *ast = (Xen_AST_Node *)__instance_new(&Xen_AST_Implement, nil, 0);
  if_nil_eval(ast) { return nil; }
  if (name) {
    ast->name = strdup(name);
    if (!ast->name) {
      Xen_DEL_REF(ast);
      return nil;
    }
  }
  if (value) {
    ast->value = strdup(value);
    if (!ast->value) {
      Xen_DEL_REF(ast);
      return nil;
    }
  }
  return (Xen_Instance *)ast;
}

int Xen_AST_Node_Push_Child(Xen_Instance *ast_inst, Xen_Instance *child) {
  if (Xen_TYPE(child) != &Xen_AST_Implement) { return 0; }
  Xen_AST_Node *ast = (Xen_AST_Node *)ast_inst;
  return Xen_Vector_Push(ast->children, child);
}

const char *Xen_AST_Node_Name(Xen_Instance *ast) { return ((Xen_AST_Node *)ast)->name; }

const char *Xen_AST_Node_Value(Xen_Instance *ast) { return ((Xen_AST_Node *)ast)->value; }

Xen_Instance *Xen_AST_Node_Children(Xen_Instance *ast) {
  return Xen_ADD_REF(((Xen_AST_Node *)ast)->children);
}

Xen_Instance *Xen_AST_Node_Get_Child(Xen_Instance *ast_inst, size_t index) {
  Xen_AST_Node *ast = (Xen_AST_Node *)ast_inst;
  return Xen_Operator_Eval_Pair_Steal2(ast->children, Xen_Number_From_ULong(index),
                                       Xen_OPR_GET_INDEX);
}
