#include "gc_header.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_queue_instance.h"
#include "xen_typedefs.h"
#include "xen_queue.h"

Xen_Instance* Xen_Queue_New(void) {
  return __instance_new(xen_globals->implements->queue, nil, nil, 0);
}

void Xen_Queue_Grow(Xen_Instance* queue_inst) {
  Xen_Queue* queue = (Xen_Queue*)queue_inst;
  Xen_size_t new_cap = queue->capacity == 0 ? 8 : queue->capacity * 2;
  Xen_GCHandle** new_buf = Xen_Alloc(new_cap * sizeof(Xen_GCHandle*));
  if (queue->size != 0) {
    Xen_size_t mask = queue->capacity - 1;
    for (Xen_size_t i = 0; i < queue->size; i++) {
      Xen_size_t idx = (queue->head + i) & mask;
      new_buf[i] = queue->buf[idx];
    }
  }
  Xen_Dealloc(queue->buf);
  queue->buf = new_buf;
  queue->capacity = new_cap;
  queue->head = 0;
  queue->tail = queue->size;
}

void Xen_Queue_Push(Xen_Instance* queue_inst, Xen_Instance* value) {
  Xen_Queue* queue = (Xen_Queue*)queue_inst;
  if (queue->size >= queue->capacity) Xen_Queue_Grow(queue_inst);
  Xen_size_t mask = queue->capacity - 1;
  queue->buf[queue->tail] = Xen_GCHandle_New((Xen_GCHeader*)queue);
  Xen_GC_Write_Field(&queue->buf[queue->tail], (struct __GC_Header *)value);
  queue->tail = (queue->tail + 1) & mask;
  queue->size++;
}

Xen_Instance* Xen_Queue_Pop(Xen_Instance* queue_inst) {
  Xen_Queue* queue = (Xen_Queue*)queue_inst;
  if (queue->size == 0) return NULL;
  Xen_size_t mask = queue->capacity - 1;
  Xen_Instance* value = (Xen_Instance*)queue->buf[queue->head]->ptr;
  Xen_GCHandle_Free(queue->buf[queue->head]);
  queue->head = (queue->head + 1) & mask;
  queue->size--;
  return value;
}
