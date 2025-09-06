#ifndef __XEN_MAP_H__
#define __XEN_MAP_H__

#include "instance.h"
typedef struct {
  Xen_Instance *key;
  Xen_Instance *value;
} Xen_Map_Pair;

typedef struct {
  char *key;
  Xen_Instance *value;
} Xen_Map_Pair_Str;

int Xen_Map_Push_Pair(Xen_Instance *, Xen_Map_Pair);
int Xen_Map_Push_Pair_Str(Xen_Instance *, Xen_Map_Pair_Str);

#endif
