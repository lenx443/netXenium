#include "xen_except.h"
#include "instance.h"
#include "xen_cstrings.h"
#include "xen_except_implement.h"
#include "xen_except_instance.h"
#include "xen_nil.h"
#include "xen_typedefs.h"
#include <stdlib.h>

Xen_Instance* Xen_Except_New(Xen_c_string_t type, Xen_c_string_t message) {
  Xen_Except* except =
      (Xen_Except*)__instance_new(&Xen_Except_Implement, nil, nil, 0);
  if (!except) {
    return NULL;
  }
  if (!type) {
    return NULL;
  }
  except->type = Xen_CString_Dup(type);
  if (!except->type) {
    abort();
  }
  if (message) {
    except->message = Xen_CString_Dup(message);
    if (!except->type) {
      abort();
    }
  }
  return (Xen_Instance*)except;
}
