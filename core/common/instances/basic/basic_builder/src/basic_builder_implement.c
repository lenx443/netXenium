#include "basic_builder_implement.h"
#include "basic.h"
#include "basic_builder_instance.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_nil.h"
#include "xen_typedefs.h"

static Xen_Instance* basic_builder_destroy(Xen_Instance* self,
                                           Xen_Instance* args,
                                           Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Dealloc((Xen_string_t)((Xen_Basic_Builder*)self)->name);
  return nil;
}

Xen_Implement Xen_Basic_Builder_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "BasicBuilder",
    .__inst_size = sizeof(struct Xen_Basic_Builder_Instance),
    .__inst_default_flags = XEN_INSTANCE_FLAG_MAPPED,
    .__inst_trace = Xen_Basic_Mapped_Trace,
    .__props = &Xen_Nil_Def,
    .__alloc = NULL,
    .__create = NULL,
    .__destroy = basic_builder_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = NULL,
    .__set_attr = NULL,
};
