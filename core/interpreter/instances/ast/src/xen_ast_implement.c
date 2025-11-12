#include "xen_ast_implement.h"
#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_alloc.h"
#include "xen_ast_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

static Xen_Instance* ast_alloc(ctx_id_t id, Xen_Instance* self,
                               Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast = (Xen_AST_Node*)Xen_Instance_Alloc(&Xen_AST_Implement);
  ast->name = NULL;
  ast->value = NULL;
  ast->children = Xen_Vector_New();
  if (!ast->children) {
    Xen_DEL_REF(ast);
    return NULL;
  }
  return (Xen_Instance*)ast;
}

static Xen_Instance* ast_destroy(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast = (Xen_AST_Node*)self;
  if (ast->name)
    Xen_Dealloc((void*)ast->name);
  if (ast->name)
    Xen_Dealloc((void*)ast->value);
  Xen_DEL_REF(ast->children);
  return nil;
}

static Xen_Instance* ast_string(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<AST>");
  if (!string) {
    return NULL;
  }
  return string;
}

Xen_Implement Xen_AST_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, 0x00),
    .__impl_name = "AST",
    .__inst_size = sizeof(struct Xen_AST_Node_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = ast_alloc,
    .__create = NULL,
    .__destroy = ast_destroy,
    .__string = ast_string,
    .__raw = ast_string,
    .__callable = NULL,
    .__hash = NULL,
};
