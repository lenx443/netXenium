#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "vm_consts.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

vm_Consts_ptr vm_consts_new() {
  vm_Consts_ptr consts = malloc(sizeof(vm_Consts));
  if (!consts) {
    return NULL;
  }
  consts->c_names = Xen_Vector_New();
  if_nil_eval(consts->c_names) {
    return NULL;
  }
  consts->c_instances = Xen_Vector_New();
  if_nil_eval(consts->c_instances) {
    return NULL;
  }
  return consts;
}

vm_Consts_ptr vm_consts_from_values(struct __Instance* c_names,
                                    struct __Instance* c_instances) {
  vm_Consts_ptr consts = malloc(sizeof(vm_Consts));
  if (!consts) {
    return NULL;
  }
  consts->c_names = Xen_ADD_REF(c_names);
  consts->c_instances = Xen_ADD_REF(c_instances);
  return consts;
}

Xen_ssize_t vm_consts_push_name(vm_Consts_ptr consts, const char* c_name) {
  if (!consts || !c_name) {
    return false;
  }
  Xen_Instance* c_name_inst = Xen_String_From_CString(c_name);
  if_nil_eval(c_name_inst) {
    return false;
  }
  Xen_size_t index = Xen_SIZE(consts->c_names);
  if (!Xen_Vector_Push(consts->c_names, c_name_inst)) {
    Xen_DEL_REF(c_name_inst);
    return -1;
  }
  Xen_DEL_REF(c_name_inst);
  return index;
}

Xen_ssize_t vm_consts_push_instance(vm_Consts_ptr consts,
                                    struct __Instance* c_instance) {
  if (!consts || !c_instance) {
    return false;
  };
  Xen_size_t index = Xen_SIZE(consts->c_names);
  if (!Xen_Vector_Push(consts->c_instances, c_instance)) {
    return -1;
  }
  return index;
}

void vm_consts_free(vm_Consts_ptr consts) {
  if (!consts) {
    return;
  }
  Xen_DEL_REF(consts->c_names);
  Xen_DEL_REF(consts->c_instances);
  free(consts);
  consts = NULL;
}
