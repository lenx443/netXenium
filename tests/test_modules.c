#include <assert.h>

#include "instance.h"
#include "vm.h"
#include "vm_def.h"
#include "xen_function.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

int main(int argc, char** argv) {
  assert(Xen_Init(argc, argv) == 1);
  Xen_Instance* echo_cmd =
      vm_get_instance("echo", vm->vm_ctx_stack->ctx->ctx_id);
  assert(echo_cmd != NULL);
  Xen_Instance* text = Xen_String_From_CString("Hola Mundo\n");
  assert(Xen_Nil_NEval(text));
  Xen_Instance* args = Xen_Vector_From_Array(1, &text);
  assert(Xen_Nil_NEval(args));
  Xen_DEL_REF(text);
  assert(Xen_Function_Call(echo_cmd, args) != NULL);
  Xen_DEL_REF(args);
  Xen_DEL_REF(echo_cmd);
  Xen_Finish();
  return 0;
}
