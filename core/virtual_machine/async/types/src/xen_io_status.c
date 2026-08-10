#include "xen_io_status.h"
#include "async.h"
#include "coroutine.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_eventloop.h"
#include "xen_eventloop_instance.h"
#include "xen_except.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_timer.h"
#include "xen_timer_heap.h"

#include <sys/epoll.h>
#include <unistd.h>

static void io_status_trace(Xen_GCHeader* h) {
  Xen_IO_Status* io = (Xen_IO_Status*)h;
  Xen_GC_Trace_GCHeader(io->evloop);
  if (io->in->ptr) Xen_GC_Trace_GCHeader(io->in);
  if (io->out->ptr) Xen_GC_Trace_GCHeader(io->out);
  if (io->in_timer->ptr) Xen_GC_Trace_GCHeader(io->in_timer);
  if (io->out_timer->ptr) Xen_GC_Trace_GCHeader(io->out_timer);
}

static void io_status_destroy(Xen_GCHeader* h) {
  Xen_IO_Status* io = (Xen_IO_Status*)h;
  Xen_IO_Status_Close(io);
  Xen_GCHandle_Free(io->evloop);
  Xen_GCHandle_Free(io->in);
  Xen_GCHandle_Free(io->out);
  Xen_GCHandle_Free(io->in_timer);
  Xen_GCHandle_Free(io->out_timer);
}

static void io_status_in_timer_callback(void* io_ptr) {
  Xen_IO_Status* io = io_ptr;
  if (io->in_status)
    *io->in_status = XEN_IO_STATUS_TIMEOUT;
}

static void io_status_out_timer_callback(void* io_ptr) {
  Xen_IO_Status* io = io_ptr;
  if (io->out_status)
    *io->out_status = XEN_IO_STATUS_TIMEOUT;
}

Xen_IO_Status* Xen_IO_Status_New(void* fd) {
  if (!Xen_VM()->evloop.active) {
    Xen_AsyncError();
    return NULL;
  }
  Xen_IO_Status* io = (Xen_IO_Status*)Xen_GC_New(
    sizeof(struct IO_Status), io_status_trace, io_status_destroy);
#ifdef __linux
  io->fd = *(int*)fd;
#endif
  Xen_Instance* evloop = (Xen_Instance*)Xen_VM()->evloop.evloop->ptr;
  io->evloop = Xen_GCHandle_New_From((Xen_GCHeader*)io, (Xen_GCHeader*)evloop);
  io->in = Xen_GCHandle_New((Xen_GCHeader*)io);
  io->out = Xen_GCHandle_New((Xen_GCHeader*)io);
  io->in_timer = Xen_GCHandle_New((Xen_GCHeader*)io);
  io->out_timer = Xen_GCHandle_New((Xen_GCHeader*)io);
  struct epoll_event event;
  event.events = EPOLLHUP | EPOLLERR;
  event.data.ptr = io;
  epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_ADD, *(int*)fd, &event);
  io->events = event.events;
  return io;
}

void* Xen_IO_Status_FD(Xen_IO_Status* io) {
  return &io->fd;
}

void Xen_IO_Status_SIn(Xen_IO_Status* io, Xen_Instance* in, int* status) {
  if (status) {
    *status = XEN_IO_STATUS_WAIT;
    io->in_status = status;
  }
  Xen_Instance* evloop = (Xen_Instance*)io->evloop->ptr;
  if (!(io->events & EPOLLIN)) {
    struct epoll_event event;
    event.data.ptr = io;
    event.events = io->events;
    event.events |= EPOLLIN;
    epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_MOD, io->fd, &event);
    io->events = event.events;
  }
  Xen_IGC_Write_Field(&io->in, in);
  Xen_EventLoop_IO_Inc(evloop);
}

void Xen_IO_Status_SOut(Xen_IO_Status* io, Xen_Instance* out, int* status) {
  if (status) {
    *status = XEN_IO_STATUS_WAIT;
    io->out_status = status;
  }
  Xen_Instance* evloop = (Xen_Instance*)io->evloop->ptr;
  if (!(io->events & EPOLLOUT)) {
    struct epoll_event event;
    event.data.ptr = io;
    event.events = io->events;
    event.events |= EPOLLOUT;
    epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_MOD, io->fd, &event);
    io->events = event.events;
  }
  Xen_IGC_Write_Field(&io->out, out);
  Xen_EventLoop_IO_Inc(evloop);
}

void Xen_IO_Status_SIn_Timer(Xen_IO_Status* io, Xen_uint64_t delay) {
  Xen_Instance* timer = Xen_Timer_New((Xen_Instance*)io->in->ptr, Xen_Timer_Now_MS() + delay, io_status_in_timer_callback, io);
  Xen_Async_Scheduler_Timer((Xen_Instance*)io->evloop->ptr, timer);
  if (io->in_timer->ptr) {
    Xen_Timer_Heap_Remove((Xen_Timer_Heap*)((Xen_EventLoop*)io->evloop)->timer_heap->ptr,
                          (Xen_Instance*)io->out_timer->ptr);
  }
}

void Xen_IO_Status_SOut_Timer(Xen_IO_Status* io, Xen_uint64_t delay) {
  Xen_Instance* timer = Xen_Timer_New((Xen_Instance*)io->out->ptr, Xen_Timer_Now_MS() + delay, io_status_out_timer_callback, io);
  Xen_Async_Scheduler_Timer((Xen_Instance*)io->evloop->ptr, timer);
}

void Xen_IO_Status_In_Wake(Xen_IO_Status* io) {
  if (io->in_status) *io->in_status = XEN_IO_STATUS_READY;
  Xen_Instance* evloop = (Xen_Instance*)io->evloop->ptr;
  Xen_Instance* coro = (Xen_Instance*)io->in->ptr;
  io->in->ptr = NULL;
  if (io->out_timer->ptr) {
    Xen_Timer_Heap_Remove((Xen_Timer_Heap*)((Xen_EventLoop*)io->evloop)->timer_heap->ptr,
                          (Xen_Instance*)io->in_timer->ptr);
    io->in_timer->ptr = NULL;
  }
  Xen_Coroutine_SStatus(coro, Xen_CORO_RESUME);
  Xen_EventLoop_Task_Push(evloop, coro);
  Xen_EventLoop_IO_Dec(evloop);
}

void Xen_IO_Status_Out_Wake(Xen_IO_Status* io) {
  if (io->out_status) *io->out_status = XEN_IO_STATUS_READY;
  Xen_Instance* evloop = (Xen_Instance*)io->evloop->ptr;
  Xen_Instance* coro = (Xen_Instance*)io->out->ptr;
  io->out->ptr = NULL;
  if (io->out_timer->ptr) {
    Xen_Timer_Heap_Remove((Xen_Timer_Heap*)((Xen_EventLoop*)io->evloop)->timer_heap->ptr,
                          (Xen_Instance*)io->out_timer->ptr);
    io->out_timer->ptr = NULL;
  }
  Xen_Coroutine_SStatus(coro, Xen_CORO_RESUME);
  Xen_EventLoop_Task_Push(evloop, coro);
  Xen_EventLoop_IO_Dec(evloop);
}

void Xen_IO_Status_In_Clear(Xen_IO_Status* io) {
  Xen_Instance* evloop = (Xen_Instance*)io->evloop->ptr;
  if (io->in->ptr) {
    Xen_EventLoop_IO_Dec(evloop);
    io->in->ptr = NULL;
  }
  if (io->in_timer->ptr) io->in_timer->ptr = NULL;
  if (io->in_status) io->in_status = NULL;
  if (io->events & EPOLLIN) {
    struct epoll_event event;
    event.data.ptr = io;
    event.events = io->events;
    event.events &= ~(EPOLLIN);
    epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_MOD, io->fd, &event);
    io->events = event.events;
  }
}

void Xen_IO_Status_Out_Clear(Xen_IO_Status* io) {
  Xen_Instance* evloop = (Xen_Instance*)io->evloop->ptr;
  if (io->out->ptr) {
    Xen_EventLoop_IO_Dec(evloop);
    io->out->ptr = NULL;
  }
  if (io->out_timer->ptr) io->out_timer->ptr = NULL;
  if (io->out_status) io->out_status = NULL;
  if (io->events & EPOLLOUT) {
    struct epoll_event event;
    event.data.ptr = io;
    event.events = io->events;
    event.events &= ~(EPOLLOUT);
    epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_MOD, io->fd, &event);
    io->events = event.events;
  }
}

void Xen_IO_Status_Wake(Xen_IO_Status* io) {
  if (io->in->ptr) Xen_IO_Status_In_Wake(io);
  if (io->out->ptr) Xen_IO_Status_Out_Wake(io);
}

void Xen_IO_Status_Close(Xen_IO_Status* io) {
  if (io->closed) return;
  close(io->fd);
  io->closed = 1;
}
