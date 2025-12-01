#include "instance_life.h"
#include "xen_boolean_implement.h"
#include "xen_igc.h"
#include "xen_map_implement.h"
#include "xen_method_implement.h"
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

Instance_Life Instances[] = {
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
};

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
