#ifndef __STRING_UTF8_h__
#define __STRING_UTF8_h__

#include <stddef.h>

#include "list.h"

typedef struct {
  char ch[4];
  int size;
} CharUTF8;

int char_utf8_display_with(CharUTF8);

LIST_ptr string_utf8_new(char *);
int string_utf8_push_back(LIST_ptr, char *);
char *string_utf8_get(LIST_ptr);
CharUTF8 string_utf8_index_get(LIST_ptr, int);
int string_utf8_display_width(LIST_ptr);
int string_utf8_strcmp_cstring(LIST_ptr, char *);

#endif
