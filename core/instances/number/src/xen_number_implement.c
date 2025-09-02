#include <stdlib.h>

#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"
#include "xen_register.h"
#include "xen_string.h"

static int number_alloc(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  Xen_Number *num = (Xen_Number *)self;
  num->digits = NULL;
  num->size = 0;
  num->sign = 0;
  return 1;
}

static int number_destroy(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  Xen_Number *num = (Xen_Number *)self;
  if (num->digits) free(num->digits);
  return 1;
}

static int number_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  const char *cstring = Xen_Number_As_CString(self);
  if (!cstring) { return 0; }
  Xen_Instance *string = Xen_String_From_CString(cstring);
  if (!string) {
    free((void *)cstring);
    return 0;
  }
  free((void *)cstring);
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

struct __Implement Xen_Number_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Number",
    .__inst_size = sizeof(struct Xen_Number_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = number_alloc,
    .__destroy = number_destroy,
    .__string = number_string,
    .__raw = number_string,
    .__callable = NULL,
    .__hash = NULL,
};
