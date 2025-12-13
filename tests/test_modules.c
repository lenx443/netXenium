#include <assert.h>

#include "instance.h"
#include "vm.h"
#include "vm_def.h"
#include "xen_function.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

void test_modules_run(void);

void test_modules_run(void) {
  Xen_Instance* echo_fun =
      Xen_VM_Load_Instance("echo", vm->vm_ctx_stack->ctx->ctx_id);
  assert(echo_fun != NULL);
  Xen_Instance* text = Xen_String_From_CString("Hola Mundo\n");
  assert(Xen_Nil_NEval(text));
  Xen_Instance* args = Xen_Vector_From_Array(1, &text);
  assert(Xen_Nil_NEval(args));
  assert(Xen_Function_Call(echo_fun, args, nil) != NULL);
}
