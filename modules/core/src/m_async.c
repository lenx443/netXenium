#include "m_async.h"
#include "async.h"
#include "attrs.h"
#include "callable.h"
#include "coroutine.h"
#include "instance.h"
#include "xen_eventloop.h"
#include "xen_eventloop_instance.h"
#include "xen_except.h"
#include "xen_function.h"
#include "xen_life.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_timer.h"

#include <sys/select.h>
#include <sys/timerfd.h>

static void program_timerfd(int tfd, Xen_uint64_t next_expiration) {
    struct itimerspec ts = {0};
    Xen_uint64_t now = Xen_Timer_Now_MS();
    Xen_uint64_t delta = (next_expiration > now) ? (next_expiration - now) : 0;
    ts.it_value.tv_sec  = delta / 1000;
    ts.it_value.tv_nsec = (delta % 1000) * 1000000;
    timerfd_settime(tfd, 0, &ts, NULL);
}

static Xen_Instance*
fn_run(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ArgSpec args_def[] = {
      {"coro", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* coro_inst = Xen_Function_ArgBinding_Search(binding, "coro")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_IMPL(coro_inst) != xen_globals->implements->coroutine) {
    return NULL;
  }
  Xen_Async_Run(coro_inst);
  return nil;
}

static void fn_sleep(Xen_Instance* coro, Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_ASYNC_CLEAR_ARG_NEVER_USE
  int* step = Xen_Coroutine_Data(coro);
  if (*step == 0) {
    Xen_Function_ArgSpec args_def[] = {
        {"delay", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER, XEN_FUNCTION_ARG_REQUIRED, NULL},
        {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
    };

    Xen_Function_ArgBinding* binding =
        Xen_Function_ArgsParse(args, kwargs, args_def);
    if (!binding) {
      Xen_COROUTINE_EXCEPTED;
    }
    Xen_uint64_t delay = Xen_Number_As_UInt(Xen_Function_ArgBinding_Search(binding, "delay")->value);
    Xen_Function_ArgBinding_Free(binding);
    Xen_Instance* timer = Xen_Timer_New(coro, Xen_Timer_Now_MS() + delay);
    Xen_Instance* evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
    if (!evloop) {
      Xen_AsyncError_Already();
      Xen_COROUTINE_EXCEPTED;
    }
    Xen_EventLoop_Timer_Push(evloop, timer);
    if (timer == Xen_EventLoop_Timer_Peek(evloop)) {
      program_timerfd(((Xen_EventLoop*)evloop)->timer_fd, Xen_Timer_Expire(timer));
    }
    Xen_Coroutine_SStatus(coro, Xen_CORO_PAUSE);
    (*step)++;
    return;
  } else {
    Xen_Coroutine_SStatus(coro, Xen_CORO_TERMINATED);
    return;
  }
}

static Xen_Instance*
init(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Attr_Set_Str(self, "CORO_CREATED", Xen_Number_From_Int(Xen_CORO_CREATED));
  Xen_Attr_Set_Str(self, "CORO_TERMINATED", Xen_Number_From_Int(Xen_CORO_TERMINATED));
  Xen_Attr_Set_Str(self, "CORO_RESUME", Xen_Number_From_Int(Xen_CORO_RESUME));
  Xen_Attr_Set_Str(self, "CORO_PAUSE", Xen_Number_From_Int(Xen_CORO_PAUSE));
  return nil;
}

static Xen_Module_Function_Table functions = {
  {"run", fn_run},
  {NULL, NULL},
};

static Xen_Module_Function_Async_Table functions_async = {
  {"sleep", fn_sleep, sizeof(int)},
  {NULL, NULL, 0},
};

struct Xen_Module_Def Module_Async = {
    .mod_name = "Async",
    .mod_init = init,
    .mod_functions = functions,
    .mod_functions_async = functions_async,
    .mod_implements = NULL,
};
