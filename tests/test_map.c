#include <assert.h>
#include <stddef.h>

#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"

int main(int argc, char **argv) {
  assert(Xen_Init(argc, argv) == 1);
  {
    Xen_Instance *map = __instance_new(&Xen_Map_Implement, nil, 0);
    assert(map != NULL);
    Xen_DEL_REF(map);
  }
  {
    Xen_Instance *key = Xen_String_From_CString("age1");
    assert(Xen_Nil_NEval(key));
    Xen_Instance *foo = Xen_Number_From_Int(23);
    assert(Xen_Nil_NEval(foo));
    Xen_Instance *map =
        Xen_Map_From_Pairs_With_Size(1, &((Xen_Map_Pair){key, foo}), XEN_MAP_DEFAULT_CAP);
    assert(Xen_Nil_NEval(map));
    Xen_DEL_REF(foo);
    Xen_DEL_REF(key);
    Xen_Instance *key_access = Xen_String_From_CString("age1");
    assert(Xen_Nil_NEval(key_access));
    Xen_Instance *result = Xen_Map_Get(map, key_access);
    assert(Xen_Nil_NEval(result));
    assert(Xen_Number_As_Int(result) == 23);
    Xen_DEL_REF(result);
    Xen_DEL_REF(key_access);
    Xen_DEL_REF(map);
  }
  {
    Xen_Instance *foo = Xen_Number_From_Int(23);
    assert(Xen_Nil_NEval(foo));
    Xen_Instance *map = Xen_Map_From_Pairs_Str_With_Size(
        1, &((Xen_Map_Pair_Str){"age1", foo}), XEN_MAP_DEFAULT_CAP);
    assert(Xen_Nil_NEval(map));
    Xen_DEL_REF(foo);
    Xen_Instance *result = Xen_Map_Get_Str(map, "age1");
    assert(Xen_Nil_NEval(result));
    assert(Xen_Number_As_Int(result) == 23);
    Xen_DEL_REF(result);
    Xen_DEL_REF(map);
  }
  {
    Xen_Instance *age1 = Xen_Number_From_Int(23);
    assert(Xen_Nil_NEval(age1));
    Xen_Instance *age2 = Xen_Number_From_Int(19);
    assert(Xen_Nil_NEval(age2));
    Xen_Instance *age3 = Xen_Number_From_Int(35);
    assert(Xen_Nil_NEval(age3));
    Xen_Instance *map = Xen_Map_From_Pairs_Str_With_Size(
        3, (Xen_Map_Pair_Str[]){{"age-1", age1}, {"age-2", age2}, {"age-3", age3}},
        XEN_MAP_DEFAULT_CAP);
    assert(Xen_Nil_NEval(map));
    Xen_DEL_REF(age1);
    Xen_DEL_REF(age2);
    Xen_DEL_REF(age3);
    Xen_Instance *keys = Xen_Map_Keys(map);
    assert(Xen_Nil_NEval(keys));
    for (size_t i = 0; i < Xen_SIZE(keys); i++) {
      Xen_Instance *key = Xen_Operator_Eval_Pair_Steal2(keys, Xen_Number_From_ULong(i),
                                                        Xen_OPR_GET_INDEX);
      assert(Xen_Nil_NEval(key));
      assert(vm_call_native_function(Xen_TYPE(key)->__string, key, nil) == 1);
      Xen_Instance *key_str = xen_register_prop_get("__expose_string", 0);
      assert(Xen_Nil_NEval(key_str));
      Xen_Instance *value = Xen_Map_Get(map, key);
      assert(Xen_Nil_NEval(value));
      assert(vm_call_native_function(Xen_TYPE(value)->__string, value, nil) == 1);
      Xen_Instance *value_str = xen_register_prop_get("__expose_string", 0);
      assert(Xen_Nil_NEval(value_str));
      printf("%s: %s\n", Xen_String_As_CString(key_str),
             Xen_String_As_CString(value_str));
      Xen_DEL_REF(value_str);
      Xen_DEL_REF(key_str);
      Xen_DEL_REF(value);
      Xen_DEL_REF(key);
    }
    Xen_DEL_REF(keys);
    Xen_DEL_REF(map);
  }
  Xen_Finish();
  return 0;
}
