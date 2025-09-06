#include <assert.h>
#include <string.h>

#include "instance.h"
#include "xen_string.h"

int main() {
  Xen_Instance *string = Xen_String_From_CString("Test");
  assert(strcmp(Xen_String_As_CString(string), "Test") == 0);
  return 0;
}
