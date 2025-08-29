#include <stdlib.h>

#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_vector_implement.h"
#include "xen_vector_instance.h"

static int vector_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Vector *vector = (Xen_Vector *)self;
  vector->values = NULL;
  vector->size = 0;
  vector->capacity = 0;
  return 1;
}

static int vector_destroy(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Vector *vector = (Xen_Vector *)self;
  for (int i = 0; i < vector->size; i++) {
    Xen_DEL_REF(vector->values[i]);
  }
  free(vector->values);
  return 1;
}

struct __Implement Xen_Vector_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Vector",
    .__inst_size = sizeof(struct Xen_Vector_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = vector_alloc,
    .__destroy = vector_destroy,
    .__callable = NULL,
    .__hash = NULL,
};
