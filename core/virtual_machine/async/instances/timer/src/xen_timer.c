#include "xen_timer.h"
#include "instance.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_timer_instance.h"
#include "xen_typedefs.h"

#include <time.h>

Xen_Instance*
Xen_Timer_New(Xen_Instance* coroutine, Xen_uint64_t expire, void (*callback)(void*), void* cb_data) {
  Xen_Timer* timer = (Xen_Timer*)__instance_new(xen_globals->implements->timer, nil, nil, 0);
  Xen_IGC_Write_Field(&timer->coroutine, coroutine);
  timer->expire = expire;
  if (callback) timer->callback = callback;
  if (cb_data) timer->cb_data = cb_data;
  return (Xen_Instance*)timer;
}

Xen_Instance* Xen_Timer_Coroutine(Xen_Instance* timer) {
  return (Xen_Instance*)((Xen_Timer*)timer)->coroutine->ptr;
}

Xen_uint64_t Xen_Timer_Expire(Xen_Instance* timer) {
  return ((Xen_Timer*)timer)->expire;
}

void Xen_Timer_Callback(Xen_Instance* timer_inst) {
  Xen_Timer* timer = (Xen_Timer*)timer_inst;
  if (timer->callback)
    timer->callback(timer->cb_data);
}

void Xen_Timer_SIndex(Xen_Instance* timer, Xen_size_t index) {
  ((Xen_Timer*)timer)->index = index;
}

Xen_size_t Xen_Timer_GIndex(Xen_Instance* timer) {
  return ((Xen_Timer*)timer)->index;
}

int Xen_Timer_GCancelled(Xen_Instance* timer) {
  return ((Xen_Timer*)timer)->cancelled;
}

void Xen_Timer_SCancelled_True(Xen_Instance* timer) {
  ((Xen_Timer*)timer)->cancelled = 1;
}

void Xen_Timer_SCancelled_False(Xen_Instance* timer) {
  ((Xen_Timer*)timer)->cancelled = 0;
}

Xen_uint64_t Xen_Timer_Now_MS(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (Xen_uint64_t)ts.tv_sec * 1000ULL +
         (Xen_uint64_t)ts.tv_nsec / 1000000ULL;
}
