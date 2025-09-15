#ifndef __XEN_MAP_H__
#define __XEN_MAP_H__

#include <stddef.h>

#include "instance.h"

#define XEN_MAP_DEFAULT_CAP 128

typedef struct {
  Xen_Instance *key;
  Xen_Instance *value;
} Xen_Map_Pair;

typedef struct {
  const char *key;
  Xen_Instance *value;
} Xen_Map_Pair_Str;

Xen_Instance *Xen_Map_New(size_t);
Xen_Instance *Xen_Map_From_Pairs_With_Size(size_t, Xen_Map_Pair *, size_t);
Xen_Instance *Xen_Map_From_Pairs_Str_With_Size(size_t, Xen_Map_Pair_Str *, size_t);
int Xen_Map_Push_Pair(Xen_Instance *, Xen_Map_Pair);
int Xen_Map_Push_Pair_Str(Xen_Instance *, Xen_Map_Pair_Str);
int Xen_Map_Push_Map(Xen_Instance *, Xen_Instance *);
Xen_Instance *Xen_Map_Get(Xen_Instance *, Xen_Instance *);
Xen_Instance *Xen_Map_Get_Str(Xen_Instance *, const char *);
Xen_Instance *Xen_Map_Keys(Xen_Instance *);

#endif
