#include "xen_timer_heap.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_timer.h"
#include "xen_typedefs.h"

static void timer_heap_trace(Xen_GCHeader* obj) {
  Xen_Timer_Heap* timer_heap = (Xen_Timer_Heap*)obj;
  for (Xen_size_t i = 0; i < timer_heap->size; i++) {
    Xen_GC_Trace_GCHeader(timer_heap->timers[i]);
  }
}

static void timer_heap_destroy(Xen_GCHeader* obj) {
  Xen_Timer_Heap* timer_heap = (Xen_Timer_Heap*)obj;
  for (Xen_size_t i = 0; i < timer_heap->size; i++) {
    Xen_GCHandle_Free(timer_heap->timers[i]);
  }
  Xen_Dealloc(timer_heap->timers);
}

static void timer_heap_swap(Xen_Timer_Heap* timer_heap, Xen_size_t i, Xen_size_t j) {
  Xen_GCHeader* a = timer_heap->timers[i]->ptr;
  Xen_GCHeader* b = timer_heap->timers[j]->ptr;
  Xen_GC_Write_Field(&timer_heap->timers[i], b);
  Xen_GC_Write_Field(&timer_heap->timers[j], a);
  Xen_Timer_SIndex((Xen_Instance*)a, j);
  Xen_Timer_SIndex((Xen_Instance*)b, i);
}

static void timer_heap_up(Xen_Timer_Heap* timer_heap, Xen_size_t i) {
  while (i > 0) {
    Xen_size_t parent = (i - 1) / 2;
    if (Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[parent]->ptr) <=
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[i]->ptr)) break;
    timer_heap_swap(timer_heap, parent, i);
    i = parent;
  }
}

static void timer_heap_down(Xen_Timer_Heap* timer_heap, Xen_size_t i) {
  while (1) {
    Xen_size_t left = 2 * i + 1;
    Xen_size_t right = 2 * i + 2;
    Xen_size_t smallest = i;
    if (left < timer_heap->size &&
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[left]->ptr) <
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[smallest]->ptr))
      smallest = left;
    if (right < timer_heap->size &&
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[right]->ptr) <
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[smallest]->ptr))
      smallest = right;
    if (smallest == i) break;
    timer_heap_swap(timer_heap, i, smallest);
    i = smallest;
  }
}

Xen_Timer_Heap* Xen_Timer_Heap_New(void) {
  Xen_Timer_Heap* timer_heap = (Xen_Timer_Heap*)Xen_GC_New(
    sizeof(Xen_Timer_Heap), timer_heap_trace, timer_heap_destroy);
  timer_heap->timers = NULL;
  timer_heap->size = 0;
  timer_heap->capacity = 0;
  return timer_heap;
}

int Xen_Timer_Heap_Empty(Xen_Timer_Heap* timer_heap) {
  return timer_heap->size == 0;
}

void Xen_Timer_Heap_Push(Xen_Timer_Heap* timer_heap, Xen_Instance* timer) {
  if (timer_heap->size >= timer_heap->capacity) {
    Xen_size_t new_cap = timer_heap->capacity == 0 ? 4 : timer_heap->capacity * 2;
    timer_heap->timers = Xen_Realloc(timer_heap->timers, new_cap * sizeof(Xen_GCHandle*));
    timer_heap->capacity = new_cap;
  }
  Xen_size_t i = timer_heap->size++;
  timer_heap->timers[i] = Xen_GCHandle_New_From((Xen_GCHeader*)timer_heap,
                                                (Xen_GCHeader*)timer);
  Xen_Timer_SIndex(timer, i);
  Xen_Timer_SCancelled_False(timer);
  timer_heap_up(timer_heap, i);
}

Xen_Instance* Xen_Timer_Heap_Peek(Xen_Timer_Heap* timer_heap) {
  return (timer_heap->size > 0) ? (Xen_Instance*)timer_heap->timers[0]->ptr : NULL;
}

Xen_Instance* Xen_Timer_Heap_Pop(Xen_Timer_Heap* timer_heap) {
  if (timer_heap->size == 0) {
    return NULL;
  }
  Xen_Instance* top = (Xen_Instance*)timer_heap->timers[0]->ptr;
  timer_heap->size--;
  if (timer_heap->size > 0) {
    Xen_GC_Write_Field(&timer_heap->timers[0], timer_heap->timers[timer_heap->size]->ptr);
    Xen_Timer_SIndex((Xen_Instance*)timer_heap->timers[0]->ptr, 0);
    timer_heap_down(timer_heap, 0);
  }
  return top;
}

void Xen_Timer_Heap_Remove(Xen_Timer_Heap* timer_heap, Xen_Instance* timer) {
  Xen_size_t i = Xen_Timer_GIndex(timer);
  if (i >= timer_heap->size) {
    return;
  }
  timer_heap->size--;
  if (i != timer_heap->size) {
    Xen_GC_Write_Field(&timer_heap->timers[i], timer_heap->timers[timer_heap->size]->ptr);
    Xen_Timer_SIndex((Xen_Instance*)timer_heap->timers[i]->ptr, i);
    if (i >= 0 &&
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[i]->ptr) <
        Xen_Timer_Expire((Xen_Instance *)timer_heap->timers[(i - 1) / 2])) {
      timer_heap_up(timer_heap, i);
    } else {
      timer_heap_down(timer_heap, i);
    }
  }
}
