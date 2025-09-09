#include <assert.h>

#include "instance.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_nil.h"
#include "xen_number.h"
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
  Xen_Finish();
  return 0;
}
