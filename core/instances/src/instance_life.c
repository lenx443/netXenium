#include "instance_life.h"
#include "basic_builder_implement.h"
#include "run_frame.h"
#include "xen_ast_implement.h"
#include "xen_boolean_implement.h"
#include "xen_bytes_implement.h"
#include "xen_except_implement.h"
#include "xen_function_implement.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map_implement.h"
#include "xen_method_implement.h"
#include "xen_module_implement.h"
#include "xen_nil_implement.h"
#include "xen_number_implement.h"
#include "xen_string_implement.h"
#include "xen_tuple_implement.h"
#include "xen_tuple_iterator_implement.h"
#include "xen_typedefs.h"
#include "xen_vector_implement.h"
#include "xen_vector_iterator_implement.h"

typedef int (*fn_init)(void);
typedef void (*fn_finish)(void);

typedef struct {
  fn_init init;
  fn_finish finish;
} Instance_Life;

static struct __Implement_Pointers implements;

static Instance_Life Instances[] = {
    {Xen_Number_Init, Xen_Number_Finish},
    {Xen_String_Init, Xen_String_Finish},
    {Xen_Vector_Init, Xen_Vector_Finish},
    {Xen_Vector_Iterator_Init, Xen_Vector_Iterator_Finish},
    {Xen_Tuple_Init, Xen_Tuple_Finish},
    {Xen_Tuple_Iterator_Init, Xen_Tuple_Iterator_Finish},
    {Xen_Map_Init, Xen_Map_Finish},
    {Xen_Boolean_Init, Xen_Boolean_Finish},
    {Xen_Nil_Init, Xen_Nil_Finish},
    {Xen_Method_Init, Xen_Method_Finish},
    {Xen_Except_Init, Xen_Except_Finish},
    {Xen_Bytes_Init, Xen_Bytes_Finish},
    {Xen_AST_Init, Xen_AST_Finish},
};

void Xen_Instance_GetReady(void) {
  implements.basic_builder = Xen_Basic_Builder_GetImplement();
  implements.number = Xen_Number_GetImplement();
  implements.string = Xen_String_GetImplement();
  implements.vector = Xen_Vector_GetImplement();
  implements.vector_iterator = Xen_Vector_Iterator_GetImplement();
  implements.tuple = Xen_Tuple_GetImplement();
  implements.tuple_iterator = Xen_Tuple_Iterator_GetImplement();
  implements.map = Xen_Map_GetImplement();
  implements.boolean = Xen_Boolean_GetImplement();
  implements.nilp = Xen_Nil_GetImplement();
  implements.method = Xen_Method_GetImplement();
  implements.function = Xen_Function_GetImplement();
  implements.except = Xen_Except_GetImplement();
  implements.bytes = Xen_Bytes_GetImplement();
  implements.ast = Xen_AST_GetImplement();
  implements.run_frame = Xen_Run_Frame_GetImplement();
  implements.module = Xen_Module_GetImplement();
  xen_globals->implements = &implements;
}

int Xen_Instance_Init(void) {
  impls_maps = Xen_IGC_Fork_New();
  for (Xen_size_t i = 0; i < sizeof(Instances) / sizeof(*Instances); i++) {
    if (!Instances[i].init()) {
      while (i-- > 0) {
        Instances[i].finish();
      }
      Xen_IGC_Pop();
      return 0;
    }
  }
  return 1;
}

void Xen_Instance_Finish(void) {
  for (Xen_size_t i = sizeof(Instances) / sizeof(*Instances); i-- > 0;) {
    Instances[i].finish();
  }
  Xen_IGC_Pop();
}

Xen_IGC_Fork* impls_maps = NULL;
