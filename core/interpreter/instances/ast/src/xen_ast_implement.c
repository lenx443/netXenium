#include "xen_ast_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "source_file.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_ast_instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_vector.h"

static void ast_trace(Xen_GCHeader* h) {
  Xen_AST_Node* ast = (Xen_AST_Node*)h;
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)ast->children);
}

static Xen_Instance* ast_alloc(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast =
      (Xen_AST_Node*)Xen_Instance_Alloc(xen_globals->implements->ast);
  ast->name = NULL;
  ast->value = NULL;
  ast->sta = (Xen_Source_Address){0};
  ast->children = Xen_Vector_New();
  if (!ast->children) {
    return NULL;
  }
  return (Xen_Instance*)ast;
}

static Xen_Instance* ast_destroy(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast = (Xen_AST_Node*)self;
  if (ast->name)
    Xen_Dealloc((void*)ast->name);
  if (ast->name)
    Xen_Dealloc((void*)ast->value);
  return nil;
}

static Xen_Instance* ast_string(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<AST>");
  if (!string) {
    return NULL;
  }
  return string;
}

static Xen_Instance* ast_get_index(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* index = Xen_Tuple_Get_Index(args, 0);
  Xen_AST_Node* ast = (Xen_AST_Node*)self;
  Xen_Instance* value =
      Xen_Vector_Get_Index(ast->children, Xen_Number_As_ULong(index));
  return value;
}

static Xen_Instance* ast_name(Xen_Instance* self, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast = (Xen_AST_Node*)self;
  return Xen_String_From_CString(ast->name);
}

static Xen_Instance* ast_value(Xen_Instance* self, Xen_Instance* args,
                               Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast = (Xen_AST_Node*)self;
  if (!ast->value) {
    return nil;
  }
  return Xen_String_From_CString(ast->value);
}

static Xen_Instance* ast_children(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_AST_Node* ast = (Xen_AST_Node*)self;
  return ast->children;
}

Xen_Implement __AST_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, 0x00),
    .__impl_name = "AST",
    .__inst_size = sizeof(struct Xen_AST_Node_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = ast_trace,
    .__props = NULL,
    .__alloc = ast_alloc,
    .__create = NULL,
    .__destroy = ast_destroy,
    .__string = ast_string,
    .__raw = ast_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

struct __Implement* Xen_AST_GetImplement(void) {
  return &__AST_Implement;
}

int Xen_AST_Init(void) {
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__get_index", ast_get_index, nil) ||
      !Xen_VM_Store_Native_Function(props, "name", ast_name, nil) ||
      !Xen_VM_Store_Native_Function(props, "value", ast_value, nil) ||
      !Xen_VM_Store_Native_Function(props, "children", ast_children, nil)) {
    return 0;
  }
  __AST_Implement.__props = props;
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_AST_Finish(void) {}
