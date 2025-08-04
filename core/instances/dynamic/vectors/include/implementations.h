#ifndef __IMPLEMENTATIONS_H__
#define __IMPLEMENTATIONS_H__

#include <stdbool.h>
#include <stddef.h>

#include "implement.h"

struct __Implementations {
  struct __Implement **__impls;
  size_t __size;
  size_t __capacity;
};

struct __Implementations *__implementations_new();
bool __implementations_push(struct __Implementations *, struct __Implement *);
void __implementations_free(struct __Implementations *);

#endif
