#include "xen_set.h"
#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_set_implement.h"
#include "xen_set_instance.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include <string.h>

Xen_Instance* Xen_Set_New() {
  return __instance_new(&Xen_Set_Implement, nil, nil, 0);
}

Xen_Instance* Xen_Set_From_Array(Xen_size_t size, Xen_Instance** array) {
  Xen_Instance* set = Xen_Set_New();
  if (!set) {
    return NULL;
  }
  for (Xen_size_t i = 0; i < size; i++) {
    if (!Xen_Set_Push(set, array[i])) {
      Xen_DEL_REF(set);
      return NULL;
    }
  }
  return set;
}

Xen_Instance* Xen_Set_From_Array_Str(Xen_size_t size, const char** array) {
  Xen_Instance* set = Xen_Set_New();
  if (!set) {
    return NULL;
  }
  for (Xen_size_t i = 0; i < size; i++) {
    if (!Xen_Set_Push_Str(set, array[i])) {
      Xen_DEL_REF(set);
      return NULL;
    }
  }
  return set;
}

int Xen_Set_Push(Xen_Instance* set_inst, Xen_Instance* value) {
  if (!value || !Xen_IMPL(value)->__hash) {
    return 0;
  }
  Xen_Set* set = (Xen_Set*)set_inst;

  Xen_Instance* hash_inst =
      Xen_VM_Call_Native_Function(value->__impl->__hash, value, nil, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return 0;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % set->capacity;
  Xen_DEL_REF(hash_inst);
  struct __Set_Node* current = set->buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_IMPL(value) == &Xen_String_Implement) {
      if (Xen_IMPL(current->value) == &Xen_String_Implement &&
          strcmp(((Xen_String*)current->value)->characters,
                 ((Xen_String*)value)->characters) == 0) {
        eval = Xen_True;
      } else {
        eval = Xen_False;
      }
    } else {
      eval = Xen_Operator_Eval_Pair(current->value, value, Xen_OPR_EQ);
      if (!eval) {
        return 0;
      }
    }
    if (eval == Xen_True) {
      return 1;
    }
    current = current->next;
  }
  struct __Set_Node* new_node = Xen_Alloc(sizeof(struct __Set_Node));
  if (!new_node) {
    return 0;
  }
  if (!Xen_Vector_Push(set->values, value)) {
    Xen_Dealloc(new_node);
    return 0;
  }
  new_node->value = value;
  Xen_ADD_REF(value);

  new_node->next = set->buckets[hash_index];
  set->buckets[hash_index] = new_node;
  return 1;
}

int Xen_Set_Push_Str(Xen_Instance* set, const char* value) {
  if (!value) {
    return 0;
  }
  Xen_Instance* value_inst = Xen_String_From_CString(value);
  if (!value_inst) {
    return 0;
  }
  if (!Xen_Set_Push(set, value_inst)) {
    Xen_DEL_REF(value_inst);
    return 0;
  }
  Xen_DEL_REF(value_inst);
  return 1;
}

int Xen_Set_Has(Xen_Instance* set_inst, Xen_Instance* value) {
  Xen_Set* set = (Xen_Set*)set_inst;
  Xen_Instance* hash_inst =
      Xen_VM_Call_Native_Function(value->__impl->__hash, value, nil, nil);
  if (!hash_inst || Xen_Nil_Eval(hash_inst)) {
    return 0;
  }

  unsigned long hash_index = Xen_Number_As_ULong(hash_inst) % set->capacity;
  Xen_DEL_REF(hash_inst);
  struct __Set_Node* current = set->buckets[hash_index];
  while (current) {
    Xen_Instance* eval = nil;
    if (Xen_IMPL(value) == &Xen_String_Implement) {
      if (Xen_IMPL(current->value) == &Xen_String_Implement &&
          strcmp(((Xen_String*)current->value)->characters,
                 ((Xen_String*)value)->characters) == 0) {
        eval = Xen_True;
      } else {
        eval = Xen_False;
      }
    } else {
      eval = Xen_Operator_Eval_Pair(current->value, value, Xen_OPR_EQ);
      if (!eval) {
        return 0;
      }
    }
    if (eval == Xen_True) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}
