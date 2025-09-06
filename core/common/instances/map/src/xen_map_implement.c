#include <stdlib.h>

#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_map_implement.h"
#include "xen_map_instance.h"
#include "xen_register.h"
#include "xen_string.h"

static int map_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Map *map = (Xen_Map *)self;
  map->map_buckets = NULL;
  map->map_capacity = 0;
  return 1;
}

static int map_destroy(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Map *map = (Xen_Map *)self;
  if (map->map_buckets) {
    for (int i = 0; i < map->map_capacity; i++) {
      struct __Map_Node *current = map->map_buckets[i];
      while (current) {
        struct __Map_Node *temp = current;
        current = current->next;
        Xen_DEL_REF(temp->key);
        Xen_DEL_REF(temp->value);
        free(temp);
      }
    }
    free(map->map_buckets);
  }
  return 1;
}

static int map_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Instance *string = Xen_String_From_CString("<Map>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

struct __Implement Xen_Map_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Map",
    .__inst_size = sizeof(struct Xen_Map_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = map_alloc,
    .__destroy = map_destroy,
    .__opr = {NULL},
    .__string = map_string,
    .__raw = map_string,
    .__callable = NULL,
    .__hash = NULL,
};
