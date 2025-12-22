#include "xen_bytes.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_bytes_instance.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_typedefs.h"

Xen_bool_t Xen_IsBytes(Xen_Instance* inst) {
  return Xen_IMPL(inst) == xen_globals->implements->bytes;
}

Xen_Instance* Xen_Bytes_New(void) {
  return __instance_new(xen_globals->implements->bytes, nil, nil, 0);
}

Xen_Instance* Xen_Bytes_From_Array(Xen_size_t size, Xen_uint8_t* arr) {
  Xen_Instance* bytes = Xen_Bytes_New();
  Xen_Bytes_Append_Array(bytes, size, arr);
  return bytes;
}

void Xen_Bytes_Append(Xen_Instance* bytes_inst, Xen_uint8_t byte) {
  Xen_Bytes* bytes = (Xen_Bytes*)bytes_inst;
  if (bytes->__size >= bytes->capacity) {
    Xen_size_t new_cap = (bytes->capacity == 0) ? 8 : bytes->capacity * 2;
    bytes->bytes = Xen_Realloc(bytes->bytes, new_cap);
    bytes->capacity = new_cap;
  }
  bytes->bytes[bytes->__size++] = byte;
}

void Xen_Bytes_Append_Array(Xen_Instance* bytes, Xen_size_t size,
                            Xen_uint8_t* arr) {
  for (Xen_size_t i = 0; i < size; i++) {
    Xen_Bytes_Append(bytes, arr[i]);
  }
}

const Xen_uint8_t* Xen_Bytes_Get(Xen_Instance* bytes) {
  return ((Xen_Bytes*)bytes)->bytes;
}
