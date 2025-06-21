#include <bits/mbstate_t.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "list.h"
#include "logs.h"
#include "string_utf8.h"

int char_utf8_display_with(CharUTF8 ch) {
  wchar_t wc;
  mbstate_t state = {0};
  size_t len = mbrtowc(&wc, ch.ch, ch.size, &state);
  if (len == (size_t)-1 || len == (size_t)-2) { return -1; }
  return wcwidth(wc);
}

LIST_ptr string_utf8_new(char *str) {
  LIST_ptr new_utf8 = list_new();
  string_utf8_push_back(new_utf8, str);
  return new_utf8;
}

int string_utf8_push_back(LIST_ptr string_utf8, char *str) {
  unsigned char *p = (unsigned char *)str;

  while (*p != '\0') {
    CharUTF8 ch = {0};
    int size = 0;

    if ((p[0] & 0x80) == 0x00)
      size = 1;
    else if ((p[0] & 0xE0) == 0xC0)
      size = 2;
    else if ((p[0] & 0xF0) == 0xE0)
      size = 3;
    else if ((p[0] & 0xF8) == 0xF0)
      size = 4;

    int valid = 1;
    for (int i = 1; i < size; ++i) {
      if ((p[i] & 0xC0) != 0x80) {
        valid = 0;
        break;
      }
    }

    if (size > 0 && valid) {
      for (int i = 0; i < size; ++i)
        ch.ch[i] = *p++;
      ch.size = size;
      if (!list_push_back(string_utf8, &ch, sizeof(CharUTF8))) {
        DynSetLog(NULL);
        log_add(NULL, ERROR, "String-UTF8", "No se pudo agregar el elemento a la cadena");
        return 0;
      }
    } else
      p++;
  }
  return 1;
}

char *string_utf8_get(LIST_ptr utf8) {
  char *out_str;
  NODE_ptr node = NULL;
  int total_size = 0;

  FOR_EACH(&node, *utf8) {
    CharUTF8 *character = (CharUTF8 *)node->point;
    total_size += character->size;
  }

  char *result = malloc(total_size + 1);
  if (!result) {
    log_add(NULL, ERROR, "String-UTF8",
            "No,se pudo obtener la cadena por falta de memoria");
    return NULL;
  }

  node = NULL;
  int j = 0;
  FOR_EACH(&node, *utf8) {
    CharUTF8 *character = (CharUTF8 *)node->point;
    for (int i = 0; i < character->size; i++)
      result[j++] = character->ch[i];
  }
  result[j] = '\0';

  out_str = result;
  return out_str;
}

CharUTF8 string_utf8_index_get(LIST_ptr utf8, int index) {
  NODE_ptr node_ch;
  if ((node_ch = list_index_get(index, *utf8)) == NULL) {
    CharUTF8 ch = {.ch[0] = '\0', .size = 1};
    return ch;
  };
  CharUTF8 *ch = (CharUTF8 *)node_ch->point;
  return *ch;
}

int string_utf8_display_width(LIST_ptr utf8) {
  int size = 0;
  NODE_ptr node = NULL;
  FOR_EACH(&node, *utf8) {
    CharUTF8 *character = (CharUTF8 *)node->point;
    size += char_utf8_display_with(*character);
  }
  return size;
}

int string_utf8_strcmp_cstring(LIST_ptr utf8, char *str) {
  if (str == NULL) return -999;
  char *cstring_utf8 = string_utf8_get(utf8);
  if (cstring_utf8 == NULL) return -999;
  int result = strcmp(cstring_utf8, str);
  free(cstring_utf8);
  return result;
}
