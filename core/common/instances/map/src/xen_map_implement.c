#include "xen_map_implement.h"
#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_boolean.h"
#include "xen_map_instance.h"
#include "xen_nil.h"
#include "xen_string.h"

static Xen_Instance* map_alloc(ctx_id_t id, Xen_Instance* self,
                               Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)self;
  map->map_keys = nil;
  map->map_buckets = NULL;
  map->map_capacity = 0;
  return Xen_True;
}

static Xen_Instance* map_destroy(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Map* map = (Xen_Map*)self;
  Xen_DEL_REF(map->map_keys);
  if (map->map_buckets) {
    for (size_t i = 0; i < map->map_capacity; i++) {
      struct __Map_Node* current = map->map_buckets[i];
      while (current) {
        struct __Map_Node* temp = current;
        current = current->next;
        Xen_DEL_REF(temp->key);
        Xen_DEL_REF(temp->value);
        free(temp);
      }
    }
    free(map->map_buckets);
  }
  return nil;
}

static Xen_Instance* map_string(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Map>");
  if (!string) {
    return nil;
  }
  return string;
}

struct __Implement Xen_Map_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Map",
    .__inst_size = sizeof(struct Xen_Map_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__opr = {NULL},
    .__alloc = map_alloc,
    .__destroy = map_destroy,
    .__string = map_string,
    .__raw = map_string,
    .__callable = NULL,
    .__hash = NULL,
};
