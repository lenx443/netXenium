#include "xen_bytes_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_bytes_instance.h"
#include "xen_map.h"

static Xen_Implement __Bytes_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Bytes",
    .__inst_size = sizeof(struct Xen_Bytes_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = NULL,
    .__props = NULL,
    .__base = NULL,
    .__alloc = NULL,
    .__create = NULL,
    .__destroy = NULL,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

struct __Implement* Xen_Bytes_GetImplement(void) {
  return &__Bytes_Implement;
}

int Xen_Bytes_Init(void) {
  if (!Xen_VM_Store_Global("bytes",
                           (Xen_Instance*)xen_globals->implements->bytes)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  Xen_IGC_Fork_Push(impls_maps, props);
  __Bytes_Implement.__props = props;
  return 1;
}

void Xen_Bytes_Finish(void) {}
