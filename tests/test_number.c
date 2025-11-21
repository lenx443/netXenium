#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "attrs.h"
#include "instance.h"
#include "operators.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"

static void test_number_from_cstring() {
  printf("Testing Number From Strinig\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("13724", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "13724") == 0);
    assert(Xen_Number_As_Int32(foo) == 13724);
    assert(Xen_Number_As_Int64(foo) == 13724);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-13724", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-13724") == 0);
    assert(Xen_Number_As_Int32(foo) == -13724);
    assert(Xen_Number_As_Int64(foo) == -13724);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("0xFFDD11AA66CCBB99", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == INT64_MAX);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-0xFFDD11AA66CCBB99", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == INT64_MIN);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("0b010110", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "22") == 0);
    assert(Xen_Number_As_Int32(foo) == 22);
    assert(Xen_Number_As_Int64(foo) == 22);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-0b010110", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-22") == 0);
    assert(Xen_Number_As_Int32(foo) == -22);
    assert(Xen_Number_As_Int64(foo) == -22);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("0o377", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "255") == 0);
    assert(Xen_Number_As_Int32(foo) == 255);
    assert(Xen_Number_As_Int64(foo) == 255);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-0o377", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-255") == 0);
    assert(Xen_Number_As_Int32(foo) == -255);
    assert(Xen_Number_As_Int64(foo) == -255);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("02000", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "1024") == 0);
    assert(Xen_Number_As_Int32(foo) == 1024);
    assert(Xen_Number_As_Int64(foo) == 1024);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-02000", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-1024") == 0);
    assert(Xen_Number_As_Int32(foo) == -1024);
    assert(Xen_Number_As_Int64(foo) == -1024);
    Xen_Dealloc((void*)foo_str);
  }
}

static void test_number_from_cstring_base() {
  printf("Testing Number From String With Base\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("13724", 10);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "13724") == 0);
    assert(Xen_Number_As_Int32(foo) == 13724);
    assert(Xen_Number_As_Int64(foo) == 13724);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-13724", 0);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-13724") == 0);
    assert(Xen_Number_As_Int32(foo) == -13724);
    assert(Xen_Number_As_Int64(foo) == -13724);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("FFDD11AA66CCBB99", 16);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == INT64_MAX);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-FFDD11AA66CCBB99", 16);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-18436911873091484569") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == INT64_MIN);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("010110", 2);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "22") == 0);
    assert(Xen_Number_As_Int32(foo) == 22);
    assert(Xen_Number_As_Int64(foo) == 22);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-010110", 2);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-22") == 0);
    assert(Xen_Number_As_Int32(foo) == -22);
    assert(Xen_Number_As_Int64(foo) == -22);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("377", 8);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "255") == 0);
    assert(Xen_Number_As_Int32(foo) == 255);
    assert(Xen_Number_As_Int64(foo) == 255);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_CString("-377", 8);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-255") == 0);
    assert(Xen_Number_As_Int32(foo) == -255);
    assert(Xen_Number_As_Int64(foo) == -255);
    Xen_Dealloc((void*)foo_str);
  }
}

static void test_number_from_int32() {
  printf("Testing Number From Integer 32-bits\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_Int32(13724);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "13724") == 0);
    assert(Xen_Number_As_Int32(foo) == 13724);
    assert(Xen_Number_As_Int64(foo) == 13724);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_Int32(-13724);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-13724") == 0);
    assert(Xen_Number_As_Int32(foo) == -13724);
    assert(Xen_Number_As_Int64(foo) == -13724);
    Xen_Dealloc((void*)foo_str);
  }
}

static void test_number_from_int64() {
  printf("Testing Number From Integer 64-bits\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_Int64(1372418483818485888l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "1372418483818485888") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == 1372418483818485888l);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_Int64(-1372418483818485888l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-1372418483818485888") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == -1372418483818485888l);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_from_int() {
  printf("Testing Number From Int\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_Int(65535);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "65535") == 0);
    assert(Xen_Number_As_Int32(foo) == 65535);
    assert(Xen_Number_As_Int64(foo) == 65535);
    assert(Xen_Number_As_Int(foo) == 65535);
    assert(Xen_Number_As_UInt(foo) == 65535);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_Int(-65535);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-65535") == 0);
    assert(Xen_Number_As_Int32(foo) == -65535);
    assert(Xen_Number_As_Int64(foo) == -65535);
    assert(Xen_Number_As_Int(foo) == -65535);
    assert(Xen_Number_As_UInt(foo) == 0);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_from_uint() {
  printf("Testing Number From Unsigned Int\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_UInt(65535);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "65535") == 0);
    assert(Xen_Number_As_Int32(foo) == 65535);
    assert(Xen_Number_As_Int64(foo) == 65535);
    assert(Xen_Number_As_Int(foo) == 65535);
    assert(Xen_Number_As_UInt(foo) == 65535);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_from_long() {
  printf("Testing Number From Long\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_Long(2747282858672828557l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "2747282858672828557") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == 2747282858672828557l);
    assert(Xen_Number_As_Long(foo) == 2747282858672828557l);
    assert(Xen_Number_As_ULong(foo) == 2747282858672828557l);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_Long(-2747282858672828557l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-2747282858672828557") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == -2747282858672828557l);
    assert(Xen_Number_As_Long(foo) == -2747282858672828557l);
    assert(Xen_Number_As_ULong(foo) == 0l);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_from_ulong() {
  printf("Testing Number From Unsigned Long\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_ULong(2747282858672828557l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "2747282858672828557") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == 2747282858672828557l);
    assert(Xen_Number_As_Long(foo) == 2747282858672828557l);
    assert(Xen_Number_As_ULong(foo) == 2747282858672828557l);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_from_longlong() {
  printf("Testing Number From Long Long\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_LongLong(2747282858672828557l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "2747282858672828557") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == 2747282858672828557l);
    assert(Xen_Number_As_Long(foo) == 2747282858672828557l);
    assert(Xen_Number_As_ULong(foo) == 2747282858672828557l);
    assert(Xen_Number_As_LongLong(foo) == 2747282858672828557l);
    assert(Xen_Number_As_ULongLong(foo) == 2747282858672828557l);
    Xen_Dealloc((void*)foo_str);
  }
  {
    Xen_INSTANCE* foo = Xen_Number_From_LongLong(-2747282858672828557l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "-2747282858672828557") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MIN);
    assert(Xen_Number_As_Int64(foo) == -2747282858672828557l);
    assert(Xen_Number_As_Long(foo) == -2747282858672828557l);
    assert(Xen_Number_As_ULong(foo) == 0l);
    assert(Xen_Number_As_LongLong(foo) == -2747282858672828557l);
    assert(Xen_Number_As_ULongLong(foo) == 0l);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_from_ulonglong() {
  printf("Testing Number From Unsigned Long Long\n");
  {
    Xen_INSTANCE* foo = Xen_Number_From_ULongLong(2747282858672828557l);
    assert(foo != NULL);
    const char* foo_str = Xen_Number_As_CString(foo);
    assert(foo_str != NULL);
    assert(strcmp(foo_str, "2747282858672828557") == 0);
    assert(Xen_Number_As_Int32(foo) == INT32_MAX);
    assert(Xen_Number_As_Int64(foo) == 2747282858672828557l);
    assert(Xen_Number_As_Long(foo) == 2747282858672828557l);
    assert(Xen_Number_As_ULong(foo) == 2747282858672828557l);
    assert(Xen_Number_As_LongLong(foo) == 2747282858672828557l);
    assert(Xen_Number_As_ULongLong(foo) == 2747282858672828557l);
    Xen_Dealloc((void*)foo_str);
  }
}

void test_number_string() {
  printf("Testing Number String\n");
  {
    Xen_Instance* foo = Xen_Number_From_Int(27447);
    assert(Xen_Nil_NEval(foo));
    Xen_Instance* foo_str = Xen_Attr_String(foo);
    assert(foo_str && Xen_Nil_NEval(foo_str));
    assert(strcmp(Xen_String_As_CString(foo_str), "27447") == 0);
  }
}

void test_number_opr_eq() {
  printf("Testing Number Equal Operator\n");
  {
    Xen_Instance* a = Xen_Number_From_Int(1234);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_Int(1234);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_True);
  }
  {
    Xen_Instance* a = Xen_Number_From_Int(1234);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_Int(4321);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_False);
  }
  {
    Xen_Instance* a = Xen_Number_From_Int(1234);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_Int(-1234);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_False);
  }
  {
    Xen_Instance* a = Xen_Number_From_Int(0);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_Int(0);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_True);
  }
  {
    Xen_Instance* a = Xen_Number_From_CString("FFDD11AA66CCBB99", 16);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_CString("FFDD11AA66CCBB99", 16);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_True);
  }
  {
    Xen_Instance* a = Xen_Number_From_CString("FFDD11AA66CCBB99", 16);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_CString("-FFDD11AA66CCBB99", 16);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_False);
  }
  {
    Xen_Instance* a = Xen_Number_From_CString("FFDD11AA66CCBB99", 16);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_CString("FFDD11AA66CCBB993FF", 16);
    assert(b != nil);
    Xen_Instance* result = Xen_Operator_Eval_Pair(a, b, Xen_OPR_EQ);
    assert(result != nil);
    assert(result == Xen_False);
  }
}

void test_number_mul() {
  printf("Testing Number multiplication\n");
  {
    Xen_Instance* a = Xen_Number_From_Int(1234);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_Int(1234);
    assert(b != nil);
    Xen_Instance* result = Xen_Number_Mul(a, b);
    assert(result != NULL);
    assert(Xen_Number_As(int, result) == 1522756);
  }
  {
    Xen_Instance* a = Xen_Number_From_Int(1234);
    assert(a != nil);
    Xen_Instance* b = Xen_Number_From_Int(4321);
    assert(b != nil);
    Xen_Instance* result = Xen_Number_Mul(a, b);
    assert(result != NULL);
    assert(Xen_Number_As(int, result) == 5332114);
  }
}

int main(int argc, char** argv) {
  if (!Xen_Init(argc, argv)) {
    return 1;
  }
  test_number_from_cstring();
  test_number_from_cstring_base();
  test_number_from_int32();
  test_number_from_int64();
  test_number_from_int();
  test_number_from_uint();
  test_number_from_long();
  test_number_from_ulong();
  test_number_from_longlong();
  test_number_from_ulonglong();
  test_number_string();
  test_number_opr_eq();
  test_number_mul();
  Xen_Finish();
  return 0;
}
