#include <assert.h>
#include <string.h>

#include "attrs.h"
#include "instance.h"
#include "xen_life.h"
#include "xen_string.h"

int main(int argc, char** argv) {
  assert(Xen_Init(argc, argv));
  Xen_Instance* string = Xen_String_From_CString("Test");
  assert(strcmp(Xen_String_As_CString(string), "Test") == 0);
  assert(Xen_Attr_Index_Size_Get(string, 0) != NULL);
  Xen_Finish();
  return 0;
}
