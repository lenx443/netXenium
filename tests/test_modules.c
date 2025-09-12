#include <assert.h>

#include "instance.h"
#include "instances_map.h"
#include "vm_def.h"
#include "xen_command.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_module_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

int main(int argc, char **argv) {
  assert(Xen_Init(argc, argv) == 1);
  Xen_Instance *core_mod = __instances_map_get(vm->root_context->ctx_instances, "core");
  assert(Xen_Nil_NEval(core_mod));
  Xen_Instance *echo_cmd = Xen_Map_Get_Str(((Xen_Module *)core_mod)->mod_map, "echo");
  assert(Xen_Nil_NEval(echo_cmd));
  Xen_Instance *text = Xen_String_From_CString("Hola Mundo\n");
  assert(Xen_Nil_NEval(text));
  Xen_Instance *args = Xen_Vector_From_Array_With_Size(1, &text);
  assert(Xen_Nil_NEval(args));
  Xen_DEL_REF(text);
  Xen_Command_Call(echo_cmd, args);
  Xen_DEL_REF(args);
  Xen_DEL_REF(echo_cmd);
  Xen_DEL_REF(core_mod);
  Xen_Finish();
  return 0;
}
