#include "xen_io_status.h"
#include "async.h"
#include "coroutine.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_eventloop.h"
#include "xen_eventloop_instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"

#include <sys/epoll.h>
#include <unistd.h>

static void io_status_trace(Xen_GCHeader* h) {
  Xen_IO_Status* io = (Xen_IO_Status*)h;
  Xen_IO_Status_Close(io);
  if (io->in->ptr) Xen_GC_Trace_GCHeader(io->in);
  if (io->out->ptr) Xen_GC_Trace_GCHeader(io->out);
}

static void io_status_destroy(Xen_GCHeader* h) {
  Xen_IO_Status* io = (Xen_IO_Status*)h;
  Xen_GCHandle_Free(io->in);
  Xen_GCHandle_Free(io->out);
}

Xen_IO_Status* Xen_IO_Status_New(void* fd) {
  Xen_IO_Status* io = (Xen_IO_Status*)Xen_GC_New(
    sizeof(struct IO_Status), io_status_trace, io_status_destroy);
#ifdef __linux
  io->fd = *(int*)fd;
#endif
  io->in = Xen_GCHandle_New((Xen_GCHeader*)io);
  io->out = Xen_GCHandle_New((Xen_GCHeader*)io);
  Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  struct epoll_event event;
  event.events = EPOLLHUP | EPOLLERR;
  event.data.fd = *(int*)fd;
  event.data.ptr = io;
  epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_ADD, *(int*)fd, &event);
  io->events = event.events;
  Xen_EventLoop_IO_Inc(evloop);
  return io;
}

void Xen_IO_Status_SIn(Xen_IO_Status* io, Xen_Instance* in) {
  Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  struct epoll_event event;
  event.data.fd = io->fd;
  event.data.ptr = io;
  event.events = io->events;
  if (!(io->events & EPOLLIN)) {
    event.events |= EPOLLIN;
    epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_MOD, event.data.fd, &event);
    io->events = event.events;
  }
  Xen_IGC_Write_Field(&io->in, in);
}

void Xen_IO_Status_SOut(Xen_IO_Status* io, Xen_Instance* out) {
  Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  struct epoll_event event;
  event.data.fd = io->fd;
  event.data.ptr = io;
  event.events = io->events;
  if (!(io->events & EPOLLOUT)) {
    event.events |= EPOLLOUT;
    epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_MOD, event.data.fd, &event);
    io->events = event.events;
  }
  Xen_IGC_Write_Field(&io->out, out);
}

void Xen_IO_Status_In_Wake(Xen_IO_Status* io) {
  Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  Xen_Instance* coro = (Xen_Instance*)io->in->ptr;
  io->in->ptr = NULL;
  Xen_Coroutine_SStatus(coro, Xen_CORO_RESUME);
  Xen_EventLoop_Task_Push(evloop, coro);
}

void Xen_IO_Status_Out_Wake(Xen_IO_Status* io) {
  Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  Xen_Instance* coro = (Xen_Instance*)io->out->ptr;
  io->out->ptr = NULL;
  Xen_Coroutine_SStatus(coro, Xen_CORO_RESUME);
  Xen_EventLoop_Task_Push(evloop, coro);
}

void Xen_IO_Status_Wake(Xen_IO_Status* io) {
  if (io->in->ptr) Xen_IO_Status_In_Wake(io);
  if (io->out->ptr) Xen_IO_Status_Out_Wake(io);
}

void Xen_IO_Status_Close(Xen_IO_Status* io) {
  if (io->closed) return;
  Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  epoll_ctl(((Xen_EventLoop*)evloop)->event_fd, EPOLL_CTL_DEL, io->fd, NULL);
  close(io->fd);
  io->closed = 1;
  Xen_EventLoop_IO_Dec(evloop);
}
