#include <__stdarg_va_list.h>

#include "instance.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_except_implement.h"
#include "xen_except_instance.h"
#include "xen_nil.h"
#include "xen_typedefs.h"
#include <stdarg.h>
#include <stdio.h>
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

Xen_Instance* Xen_Except_New_CFormat(Xen_c_string_t type,
                                     Xen_c_string_t message, ...) {
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
    va_list args;
    va_start(args, message);
    va_list copy;
    va_copy(copy, args);
    Xen_ssize_t needed = vsnprintf(NULL, 0, message, copy);
    va_end(copy);
    if (needed < 0) {
      abort();
    }
    char* buffer = Xen_Alloc(needed + 1);
    vsnprintf(buffer, needed + 1, message, args);
    va_end(args);
    except->message = buffer;
    if (!except->type) {
      abort();
    }
  }
  return (Xen_Instance*)except;
}
