#include <stdlib.h>

#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_boolean.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_vector.h"

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

static int number_opr_eq(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  if (Xen_Nil_Eval(args) || Xen_Vector_Size(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return 0;

  Xen_Number *a = (Xen_Number *)self;
  Xen_Number *b = (Xen_Number *)Xen_Vector_Get_Index(args, 0);

  // Normalizar tamaño (ignorar ceros a la izquierda)
  size_t size_a = a->size;
  while (size_a > 0 && a->digits[size_a - 1] == 0) {
    size_a--;
  }

  size_t size_b = b->size;
  while (size_b > 0 && b->digits[size_b - 1] == 0) {
    size_b--;
  }

  // Si ambos son cero → iguales sin importar el signo
  if (size_a == 0 && size_b == 0) {
    Xen_DEL_REF(b);
    if (!xen_register_prop_set("__expose_opr_eq", Xen_True, id)) { return 0; }
    return 1;
  }

  // Signos distintos → no son iguales
  if (a->sign != b->sign) {
    Xen_DEL_REF(b);
    if (!xen_register_prop_set("__expose_opr_eq", Xen_False, id)) { return 0; }
    return 1;
  }

  // Diferente número de dígitos → no son iguales
  if (size_a != size_b) {
    Xen_DEL_REF(b);
    if (!xen_register_prop_set("__expose_opr_eq", Xen_False, id)) { return 0; }
    return 1;
  }

  // Comparar dígitos uno a uno
  for (size_t i = 0; i < size_a; i++) {
    if (a->digits[i] != b->digits[i]) {
      Xen_DEL_REF(b);
      if (!xen_register_prop_set("__expose_opr_eq", Xen_False, id)) { return 0; }
      return 1;
    }
  }

  Xen_DEL_REF(b);
  if (!xen_register_prop_set("__expose_opr_eq", Xen_True, id)) { return 0; }
  return 1;
}

struct __Implement Xen_Number_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Number",
    .__inst_size = sizeof(struct Xen_Number_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__opr = {NULL},
    .__alloc = number_alloc,
    .__destroy = number_destroy,
    .__string = number_string,
    .__raw = number_string,
    .__callable = NULL,
    .__hash = NULL,
};

int Xen_Number_Init() {
  if ((Xen_Number_Implement.__opr.__eq = callable_new_native(number_opr_eq)) == NULL) {
    return 0;
  }
  return 1;
}

void Xen_Number_Finish() { callable_free(Xen_Number_Implement.__opr.__eq); }
