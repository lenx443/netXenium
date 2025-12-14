#ifndef __INSTANCE_LIFE_H__
#define __INSTANCE_LIFE_H__

#include "implement.h"
#include "xen_igc.h"

struct __Implement_Pointers {
  Xen_Implement* basic_builder;
  Xen_Implement* number;
  Xen_Implement* string;
  Xen_Implement* vector;
  Xen_Implement* vector_iterator;
  Xen_Implement* tuple;
  Xen_Implement* tuple_iterator;
  Xen_Implement* map;
  Xen_Implement* boolean;
  Xen_Implement* nilp;
  Xen_Implement* method;
  Xen_Implement* function;
  Xen_Implement* except;
  Xen_Implement* ast;
  Xen_Implement* run_frame;
  Xen_Implement* module;
};

void Xen_Instance_GetReady(void);
int Xen_Instance_Init(void);
void Xen_Instance_Finish(void);

extern Xen_IGC_Fork* impls_maps;

#endif
