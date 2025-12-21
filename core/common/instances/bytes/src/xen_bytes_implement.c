#include <string.h>

#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_bytes_implement.h"
#include "xen_bytes_instance.h"
#include "xen_cstrings.h"
#include "xen_map.h"
#include "xen_string.h"
#include "xen_typedefs.h"

static Xen_Instance* bytes_string(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Bytes* bytes = (Xen_Bytes*)self;
  Xen_string_t buffer = Xen_CString_Dup("<Bytes('");
  Xen_size_t buflen = 10;
  for (Xen_size_t i = 0; i < Xen_SIZE(bytes); i++) {
    char byte[4];
    static const char HEX[] = "0123456789ABCDEF";
    byte[0] = '\\';
    byte[1] = 'x';
    byte[2] = HEX[(bytes->bytes[i] >> 4) & 0xF];
    byte[3] = HEX[bytes->bytes[i] & 0xF];
    buflen += 4;
    buffer = Xen_Realloc(buffer, buflen);
    strncat(buffer, byte, 4);
  }
  buflen += 3;
  buffer = Xen_Realloc(buffer, buflen);
  strcat(buffer, "')>");
  buffer[buflen - 1] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buffer);
  Xen_Dealloc(buffer);
  return string;
}

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
    .__string = bytes_string,
    .__raw = bytes_string,
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
