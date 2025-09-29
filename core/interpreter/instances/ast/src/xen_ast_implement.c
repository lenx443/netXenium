#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_ast_implement.h"
#include "xen_ast_instance.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_vector.h"

static int ast_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node *ast = (Xen_AST_Node *)self;
  ast->name = NULL;
  ast->value = NULL;
  ast->children = Xen_Vector_New();
  if_nil_eval(ast->children) { return 0; }
  return 1;
}

static int ast_destroy(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node *ast = (Xen_AST_Node *)self;
  if (ast->name) free((void *)ast->name);
  if (ast->name) free((void *)ast->value);
  Xen_DEL_REF(ast->children);
  return 1;
}

static int ast_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance *string = Xen_String_From_CString("<AST>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

Xen_Implement Xen_AST_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, 0x00),
    .__impl_name = "AST",
    .__inst_size = sizeof(struct Xen_AST_Node_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__opr = {NULL},
    .__alloc = ast_alloc,
    .__destroy = ast_destroy,
    .__string = ast_string,
    .__raw = ast_string,
    .__callable = NULL,
    .__hash = NULL,
};
