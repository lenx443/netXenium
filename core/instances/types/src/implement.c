#include "implement.h"
#include "basic.h"
#include "xen_cstrings.h"
#include "xen_gc.h"
#include "xen_nil.h"

Xen_Implement* Xen_Implement_From_Struct(Xen_ImplementStruct impl_struct) {
  Xen_Implement* impl = (Xen_Implement*)__instance_new(&Xen_Basic, nil, nil, 0);
  impl->__impl_name = Xen_CString_Dup(impl_struct.__impl_name);
  impl->__inst_size = impl_struct.__inst_size;
  impl->__inst_default_flags = impl_struct.__inst_default_flags;
  impl->__inst_trace = impl_struct.__inst_trace;
  if (impl_struct.__props) {
    Xen_GC_Write_Field((struct __GC_Header*)impl,
                       (struct __GC_Handle**)&impl->__props,
                       (struct __GC_Header*)impl_struct.__props);
  }
  if (impl_struct.__base) {
    Xen_GC_Write_Field((struct __GC_Header*)impl,
                       (struct __GC_Handle**)&impl->__base,
                       (struct __GC_Header*)impl_struct.__base);
  }
  impl->__alloc = impl_struct.__alloc;
  impl->__create = impl_struct.__create;
  impl->__destroy = impl_struct.__destroy;
  impl->__string = impl_struct.__string;
  impl->__raw = impl_struct.__raw;
  impl->__callable = impl_struct.__callable;
  impl->__hash = impl_struct.__hash;
  impl->__get_attr = impl_struct.__get_attr;
  impl->__set_attr = impl_struct.__set_attr;
  return impl;
}

void Xen_Implement_SetProps(Xen_Implement* impl, Xen_Instance* props) {
  Xen_GC_Write_Field((struct __GC_Header*)impl,
                     (struct __GC_Handle**)&impl->__props,
                     (struct __GC_Header*)props);
}
