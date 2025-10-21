#include <assert.h>
#include <string.h>

#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_string.h"

int main(int argc, char** argv) {
  assert(Xen_Init(argc, argv) == 1);
  {
    Xen_Instance* boolean = Xen_True;
    Xen_Instance* string =
        vm_call_native_function(Xen_TYPE(boolean)->__string, boolean, nil);
    assert(string && string != nil);
    assert(strcmp(Xen_String_As_CString(string), "true") == 0);
    Xen_DEL_REF(string);
    Xen_DEL_REF(boolean);
  }
  {
    Xen_Instance* boolean = Xen_False;
    Xen_Instance* string =
        vm_call_native_function(Xen_TYPE(boolean)->__string, boolean, nil);
    assert(string && string != nil);
    assert(strcmp(Xen_String_As_CString(string), "false") == 0);
    Xen_DEL_REF(string);
    Xen_DEL_REF(boolean);
  }
  Xen_Finish();
  return 0;
}
