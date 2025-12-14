#ifndef __XEN_LIFE_H__
#define __XEN_LIFE_H__

#include "gc_heap.h"
#include "instance.h"
#include "instance_life.h"
#include "program.h"
#include "source_file.h"
#include "vm_def.h"

struct Xen_Globals {
  Program_State* program;
  struct __GC_Heap* gc_heap;
  struct __IGC_Roots** igc_roots;
  Xen_Source_Table** source_table;
  struct __Implement_Pointers* implements;
  Xen_Instance* true_instance;
  Xen_Instance* false_instance;
  Xen_Instance* nil_instance;
  VM** vm;
};

void Xen_GetReady(void*);
int Xen_Init(int, char**);
void Xen_Finish(void);

extern struct Xen_Globals* xen_globals;

#endif
