#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "instance.h"
#include "xen_number.h"

static void test_number_from_cstring() {
  printf("Testing Number From Strinig\n");
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("13724", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "13724") == 0);
    assert(Xen_Number_As_Int32(foo) == 13724);
    assert(Xen_Number_As_Int64(foo) == 13724);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-13724", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-13724") == 0);
    assert(Xen_Number_As_Int32(foo) == -13724);
    assert(Xen_Number_As_Int64(foo) == -13724);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("0xFFDD11AA66CCBB99", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == INT64_MAX);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-0xFFDD11AA66CCBB99", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == INT64_MIN);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("0b010110", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "22") == 0);
    assert(Xen_Number_As_Int32(foo) == 22);
    assert(Xen_Number_As_Int64(foo) == 22);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-0b010110", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-22") == 0);
    assert(Xen_Number_As_Int32(foo) == -22);
    assert(Xen_Number_As_Int64(foo) == -22);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("0o377", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "255") == 0);
    assert(Xen_Number_As_Int32(foo) == 255);
    assert(Xen_Number_As_Int64(foo) == 255);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-0o377", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-255") == 0);
    assert(Xen_Number_As_Int32(foo) == -255);
    assert(Xen_Number_As_Int64(foo) == -255);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("02000", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "1024") == 0);
    assert(Xen_Number_As_Int32(foo) == 1024);
    assert(Xen_Number_As_Int64(foo) == 1024);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-02000", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-1024") == 0);
    assert(Xen_Number_As_Int32(foo) == -1024);
    assert(Xen_Number_As_Int64(foo) == -1024);
    free((void *)foo_str);
  }
}

static void test_number_from_cstring_base() {
  printf("Testing Number From String With Base\n");
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("13724", 10);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "13724") == 0);
    assert(Xen_Number_As_Int32(foo) == 13724);
    assert(Xen_Number_As_Int64(foo) == 13724);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-13724", 0);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-13724") == 0);
    assert(Xen_Number_As_Int32(foo) == -13724);
    assert(Xen_Number_As_Int64(foo) == -13724);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("FFDD11AA66CCBB99", 16);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == INT64_MAX);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-FFDD11AA66CCBB99", 16);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == INT64_MIN);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("010110", 2);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "22") == 0);
    assert(Xen_Number_As_Int32(foo) == 22);
    assert(Xen_Number_As_Int64(foo) == 22);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-010110", 2);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-22") == 0);
    assert(Xen_Number_As_Int32(foo) == -22);
    assert(Xen_Number_As_Int64(foo) == -22);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("377", 8);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "255") == 0);
    assert(Xen_Number_As_Int32(foo) == 255);
    assert(Xen_Number_As_Int64(foo) == 255);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_CString("-377", 8);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-255") == 0);
    assert(Xen_Number_As_Int32(foo) == -255);
    assert(Xen_Number_As_Int64(foo) == -255);
    free((void *)foo_str);
  }
}

static void test_number_from_int32() {
  printf("Testing Number From Integer 32-bits\n");
  {
    Xen_INSTANCE *foo = Xen_Number_From_Int32(13724);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "13724") == 0);
    assert(Xen_Number_As_Int32(foo) == 13724);
    assert(Xen_Number_As_Int64(foo) == 13724);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_Int32(-13724);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-13724") == 0);
    assert(Xen_Number_As_Int32(foo) == -13724);
    assert(Xen_Number_As_Int64(foo) == -13724);
    free((void *)foo_str);
  }
}

static void test_number_from_int64() {
  printf("Testing Number From Integer 64-bits\n");
  {
    Xen_INSTANCE *foo = Xen_Number_From_Int64(1372418483818485888l);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "1372418483818485888") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == 1372418483818485888l);
    free((void *)foo_str);
  }
  {
    Xen_INSTANCE *foo = Xen_Number_From_Int64(-1372418483818485888l);
    assert(foo != NULL);
    const char *foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-1372418483818485888") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == -1372418483818485888l);
    free((void *)foo_str);
  }
}

int main() {
  test_number_from_cstring();
  test_number_from_cstring_base();
  test_number_from_int32();
  test_number_from_int64();
  return 0;
}
