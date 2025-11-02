#include <stddef.h>
#include <string.h>

#include "attrs.h"
#include "instance.h"
#include "xen_ast.h"
#include "xen_ast_implement.h"
#include "xen_ast_instance.h"
#include "xen_nil.h"
#include "xen_vector.h"

Xen_Instance* Xen_AST_Node_New(const char* name, const char* value) {
  Xen_AST_Node* ast = (Xen_AST_Node*)__instance_new(&Xen_AST_Implement, nil, 0);
  if (!ast) {
    return NULL;
  }
  if (name) {
    ast->name = strdup(name);
    if (!ast->name) {
      Xen_DEL_REF(ast);
      return NULL;
    }
  }
  if (value) {
    ast->value = strdup(value);
    if (!ast->value) {
      Xen_DEL_REF(ast);
      return NULL;
    }
  }
  return (Xen_Instance*)ast;
}

int Xen_AST_Node_Push_Child(Xen_Instance* ast_inst, Xen_Instance* child) {
  if (Xen_IMPL(child) != &Xen_AST_Implement) {
    return 0;
  }
  Xen_AST_Node* ast = (Xen_AST_Node*)ast_inst;
  return Xen_Vector_Push(ast->children, child);
}

const char* Xen_AST_Node_Name(Xen_Instance* ast) {
  return ((Xen_AST_Node*)ast)->name;
}

const char* Xen_AST_Node_Value(Xen_Instance* ast) {
  return ((Xen_AST_Node*)ast)->value;
}

int Xen_AST_Node_Name_Cmp(Xen_Instance* ast, const char* name) {
  return strcmp(((Xen_AST_Node*)ast)->name, name);
}

int Xen_AST_Node_Value_Cmp(Xen_Instance* ast, const char* value) {
  return strcmp(((Xen_AST_Node*)ast)->value, value);
}

size_t Xen_AST_Node_Children_Size(Xen_Instance* ast) {
  return Xen_SIZE(((Xen_AST_Node*)ast)->children);
}

Xen_Instance* Xen_AST_Node_Children(Xen_Instance* ast) {
  return Xen_ADD_REF(((Xen_AST_Node*)ast)->children);
}

Xen_Instance* Xen_AST_Node_Get_Child(Xen_Instance* ast_inst, size_t index) {
  Xen_AST_Node* ast = (Xen_AST_Node*)ast_inst;
  return Xen_Attr_Index_Size_Get(ast->children, index);
}

Xen_Instance* Xen_AST_Node_Wrap(Xen_Instance* node, const char* wrap) {
  Xen_Instance* rsult = Xen_AST_Node_New(wrap, NULL);
  if (!rsult) {
    return NULL;
  }
  if (!Xen_AST_Node_Push_Child(rsult, node)) {
    Xen_DEL_REF(rsult);
    return NULL;
  }
  return rsult;
}

#ifndef NDEBUG
static void __AST_Node_Print(Xen_Instance* ast, const char* prefix,
                             int is_last) {
  if (!ast || Xen_IMPL(ast) != &Xen_AST_Implement)
    return;

  printf("%s", prefix);
  if (is_last)
    printf("└─");
  else
    printf("├─");

  const char* name = Xen_AST_Node_Name(ast);
  const char* value = Xen_AST_Node_Value(ast);
  printf("%s", name ? name : "node");
  if (value)
    printf("='%s'\n", value);
  else
    printf("\n");

  Xen_Instance* children = Xen_AST_Node_Children(ast);
  size_t n = Xen_SIZE(children);

  char new_prefix[256];
  snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix,
           is_last ? "  " : "│ ");

  for (size_t i = 0; i < n; i++) {
    Xen_Instance* child = Xen_Attr_Index_Size_Get(children, i);
    __AST_Node_Print(child, new_prefix, i == n - 1);
    Xen_DEL_REF(child);
  }

  Xen_DEL_REF(children);
}

void Xen_AST_Node_Print(Xen_Instance* ast) {
  __AST_Node_Print(ast, "", 0);
}
#endif
