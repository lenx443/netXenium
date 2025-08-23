#include <stdlib.h>

#include "basic.h"
#include "call_args.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"

static int number_alloc(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  Xen_Number *num = (Xen_Number *)self;
  num->digits = NULL;
  num->size = 0;
  num->sign = 0;
  return 1;
}

static int number_destroy(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  Xen_Number *num = (Xen_Number *)self;
  if (num->digits) free(num->digits);
  return 1;
}

struct __Implement Xen_Number_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Number",
    .__inst_size = sizeof(struct Xen_Number_Instance),
    .__props = NULL,
    .__alloc = number_alloc,
    .__destroy = number_destroy,
    .__callable = NULL,
    .__hash = NULL,
};
