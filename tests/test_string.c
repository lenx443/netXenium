#include <assert.h>
#include <string.h>

#include "attrs.h"
#include "instance.h"
#include "xen_string.h"

void test_string_run(void);

void test_string_run(void) {
  Xen_Instance* string = Xen_String_From_CString("Test");
  assert(strcmp(Xen_String_As_CString(string), "Test") == 0);
  assert(Xen_Attr_Index_Size_Get(string, 0) != NULL);
}
