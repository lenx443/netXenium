#ifndef __XEN_MAP_H__
#define __XEN_MAP_H__

#include <stddef.h>

#include "instance.h"

typedef struct {
  Xen_Instance* key;
  Xen_Instance* value;
} Xen_Map_Pair;

typedef struct {
  const char* key;
  Xen_Instance* value;
} Xen_Map_Pair_Str;

Xen_Instance* Xen_Map_New(void);
Xen_Instance* Xen_Map_From_Pairs_With_Size(size_t, Xen_Map_Pair*);
Xen_Instance* Xen_Map_From_Pairs_Str_With_Size(size_t, Xen_Map_Pair_Str*);
int Xen_Map_Push_Pair(Xen_Instance*, Xen_Map_Pair);
int Xen_Map_Push_Pair_Str(Xen_Instance*, Xen_Map_Pair_Str);
int Xen_Map_Push_Map(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Map_Get(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Map_Get_Str(Xen_Instance*, const char*);
Xen_Instance* Xen_Map_Keys(Xen_Instance*);
int Xen_Map_Has(Xen_Instance*, Xen_Instance*);
int Xen_Map_Has_Str(Xen_Instance*, const char*);

#endif
