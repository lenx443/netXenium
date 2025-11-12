#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "list.h"
#include "logs.h"
#include "xen_alloc.h"

LIST_ptr list_new() {
  LIST_ptr list = Xen_Alloc(sizeof(LIST));
  if (list == NULL) {
    dyn_error = DYN_NO_MEMORY;
    return NULL;
  }
  dyn_error = DYN_OK;
  list->head = NULL;
  list->tail = NULL;
  return list;
}

LIST_ptr list_new_string(char* str) {
  LIST_ptr list = list_new();
  if (!list_push_back_string(list, str)) {
    return NULL;
  }
  return list;
}

int list_size(LIST list) {
  if (list_empty(&list))
    return 0;
  NODE_ptr current = list.head;
  int n = 0;
  while (current != NULL) {
    current = current->next;
    n++;
  }
  return n;
}

int list_empty(LIST_ptr list) {
  if (list == NULL || list->head == NULL) {
    dyn_error = DYN_EMPTY;
    return 1;
  }
  return 0;
}

int list_valid(LIST_ptr list) {
  if (list == NULL) {
    dyn_error = DYN_INVALID;
    return 0;
  }
  return 1;
}

int list_push_back(LIST_ptr list, const void* value, size_t size) {
  if (list == NULL) {
    dyn_error = DYN_INVALID;
    return 0;
  }
  NODE_ptr newNode = Xen_Alloc(sizeof(NODE));
  if (newNode == NULL) {
    dyn_error = DYN_NO_MEMORY;
    return 0;
  }
  newNode->size = size;
  newNode->point = Xen_Alloc(newNode->size);
  if (newNode == NULL) {
    dyn_error = DYN_NO_MEMORY;
    Xen_Dealloc(newNode);
    return 0;
  }
  memcpy(newNode->point, value, newNode->size);
  newNode->next = NULL;
  if (list->head == NULL)
    list->head = newNode;
  else
    list->tail->next = newNode;
  list->tail = newNode;
  return 1;
}

int list_push_begin(LIST_ptr list, void* value, size_t size) {
  if (list == NULL) {
    dyn_error = DYN_INVALID;
    return 0;
  }
  NODE_ptr newNode = Xen_Alloc(sizeof(NODE));
  if (newNode == NULL) {
    dyn_error = DYN_NO_MEMORY;
    return 0;
  }
  newNode->size = size;
  newNode->point = Xen_Alloc(newNode->size);
  if (newNode->point == NULL) {
    dyn_error = DYN_NO_MEMORY;
    Xen_Dealloc(newNode);
    return 0;
  }
  memcpy(newNode->point, value, newNode->size);
  newNode->next = NULL;
  if (list->head == NULL) {
    list->head = newNode;
    list->tail = newNode;
    return 1;
  }
  newNode->next = list->head;
  list->head = newNode;
  return 1;
}

int list_push_at_index(LIST_ptr list, int index, void* value, size_t size) {
  if (index < 0) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return 0;
  }
  if (index == 0 || list_empty(list)) {
    NODE_ptr node = Xen_Alloc(sizeof(NODE));
    if (!node) {
      dyn_error = DYN_NO_MEMORY;
      return 0;
    }
    node->point = Xen_Alloc(size);
    if (!node->point) {
      Xen_Dealloc(node);
      dyn_error = DYN_NO_MEMORY;
      return 0;
    }
    memcpy(node->point, value, size);
    node->size = size;
    node->next = (index == 0) ? list->head : NULL;
    if (index == 0)
      list->head = node;
    else
      list->head = node;
    return 1;
  }

  NODE_ptr prev = list->head;
  int n = 0;
  while (prev && n < index - 1) {
    prev = prev->next;
    n++;
  }

  if (!prev) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return 0;
  }

  NODE_ptr node = Xen_Alloc(sizeof(NODE));
  if (!node) {
    dyn_error = DYN_NO_MEMORY;
    return 0;
  }
  node->point = Xen_Alloc(size);
  if (!node->point) {
    Xen_Dealloc(node);
    dyn_error = DYN_NO_MEMORY;
    return 0;
  }

  memcpy(node->point, value, size);
  node->size = size;

  node->next = prev->next;
  prev->next = node;
  return 1;
}

int list_push_back_string(LIST_ptr list, char* str) {
  for (char* p = str; *p != '\0'; p++) {
    if (!list_push_back(list, p, 1))
      return 0;
  }
  return 1;
}

int list_push_back_string_node(LIST_ptr list, const char* str) {
  int size = strlen(str) + 1;
  if (!list_push_back(list, str, size))
    return 0;
  return 1;
}

NODE_ptr list_pop_back(LIST_ptr list) {
  if (list_empty(list))
    return NULL;
  NODE_ptr current = list->head;
  NODE_ptr prev = NULL;
  if (current == list->tail) {
    NODE* data = Xen_Alloc(sizeof(NODE));
    if (data == NULL) {
      dyn_error = DYN_NO_MEMORY;
      return NULL;
    }
    data->point = Xen_Alloc(current->size);
    if (data->point == NULL) {
      dyn_error = DYN_NO_MEMORY;
      Xen_Dealloc(data);
      return NULL;
    }
    memcpy(data->point, current->point, current->size);
    data->size = current->size;
    data->next = NULL;
    Xen_Dealloc(current->point);
    Xen_Dealloc(current);
    list->head = list->tail = NULL;
    return data;
  }
  while (current->next != NULL && current->next != list->tail)
    current = current->next;
  prev = current;
  current = current->next;
  NODE_ptr data = Xen_Alloc(sizeof(NODE));
  if (data == NULL) {
    dyn_error = DYN_NO_MEMORY;
    return NULL;
  }
  data->point = Xen_Alloc(current->size);
  if (data->point == NULL) {
    dyn_error = DYN_NO_MEMORY;
    Xen_Dealloc(data);
    return NULL;
  }
  memcpy(data->point, current->point, current->size);
  data->size = current->size;
  data->next = NULL;
  Xen_Dealloc(current->point);
  Xen_Dealloc(current);
  prev->next = NULL;
  list->tail = prev;
  return data;
}

void list_erase_at_index(LIST_ptr list, int index) {
  if (list_empty(list) || index < 0) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return;
  }

  NODE_ptr current = list->head;
  NODE_ptr previous = NULL;
  int n = 0;

  while (current != NULL && n < index) {
    previous = current;
    current = current->next;
    n++;
  }

  if (current == NULL)
    return;

  if (previous == NULL) {
    list->head = current->next;
  } else {
    previous->next = current->next;
  }

  node_free(&current);
}

int list_search(LIST list, void* value, size_t size) {
  if (list_empty(&list))
    return 0;
  NODE_ptr current = list.head;
  int n = 0;
  while (current != NULL) {
    if (memcmp(current->point, value, size) == 0)
      return n;
    current = current->next;
    n++;
  }
  dyn_error = DYN_ELEMENT_NO_MATCH;
  return -1;
}

int list_search_string(LIST list, const char* value) {
  if (list_empty(&list))
    return 0;
  NODE_ptr current = list.head;
  int n = 0;
  while (current != NULL) {
    char* current_str = (char*)current->point;
    if (strcmp(current_str, value) == 0)
      return n;
    current = current->next;
    n++;
  }
  dyn_error = DYN_ELEMENT_NO_MATCH;
  return -1;
}

int list_forEach(NODE_ptr* current, LIST list) {
  if (current == NULL) {
    dyn_error = DYN_ELEMENT_INVALID;
    return 0;
  }
  if (*current == NULL)
    *current = list.head;
  else
    *current = (*current)->next;
  return (*current != NULL);
}

NODE_ptr list_index_get(int index, LIST list) {
  if (list_empty(&list)) {
    return NULL;
  }
  if (index < 0) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return NULL;
  }
  NODE_ptr current = list.head;
  int n = 0;
  while (current != NULL && n < index) {
    current = current->next;
    n++;
  }
  if (current == NULL) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return NULL;
  }
  return current;
}

int list_index_set(int index, LIST_ptr list, void* value, size_t size) {
  if (list_empty(list)) {
    return 0;
  }
  if (index < 0) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return 0;
  }
  NODE_ptr current = list->head;
  NODE_ptr prev = NULL;
  int n = 0;
  while (current != NULL && n < index) {
    prev = current;
    current = current->next;
    n++;
  }
  if (current == NULL) {
    dyn_error = DYN_ELEMENT_NO_MATCH;
    return 0;
  }
  NODE_ptr newNode = Xen_Alloc(sizeof(NODE));
  if (newNode == NULL) {
    dyn_error = DYN_NO_MEMORY;
    return 0;
  }
  newNode->point = Xen_Alloc(size);
  if (newNode->point == NULL) {
    dyn_error = DYN_NO_MEMORY;
    Xen_Dealloc(newNode);
    return 0;
  }
  memcpy(newNode->point, value, size);
  newNode->size = size;
  newNode->next = current->next;
  if (prev == NULL)
    list->head = newNode;
  else
    prev->next = newNode;
  Xen_Dealloc(current->point);
  Xen_Dealloc(current);
  return 1;
}

void list_as_string(LIST list, char* str, int size) {
  int n = 0;
  NODE_ptr node = NULL;
  FOR_EACH(&node, list) {
    if (n < size - 1)
      str[n++] = *(char*)node->point;
    else
      break;
  }
  str[n] = '\0';
}

void list_clear(LIST_ptr list) {
  if (list == NULL || list->head == NULL)
    return;

  NODE_ptr current = list->head;
  while (current != NULL) {
    NODE_ptr next = current->next;
    Xen_Dealloc(current->point);
    Xen_Dealloc(current);
    current = next;
  }

  list->head = NULL;
  list->tail = NULL;
}

void list_free(LIST_ptr list) {
  if (!list_valid(list))
    return;
  NODE_ptr current = list->head;
  while (current != NULL) {
    NODE_ptr next = current->next;
    Xen_Dealloc(current->point);
    Xen_Dealloc(current);
    current = next;
  }
  Xen_Dealloc(list);
}

int node_empty(NODE_ptr* node) {
  if (node == NULL || *node == NULL) {
    dyn_error = DYN_ELEMENT_INVALID;
    return 1;
  }
  return 0;
}

void node_free(NODE_ptr* node) {
  if (node == NULL) {
    return;
  }
  Xen_Dealloc((*node)->point);
  Xen_Dealloc(*node);
  *node = NULL;
}

void DynSetLog(LIST_ptr log) {
  switch (dyn_error) {
  case DYN_OK:
    log_add(log, INFO, "Dyn-Lists", "No se ah generado ningun problema");
    break;
  case DYN_NO_MEMORY:
    log_add(log, ERROR, "Dyn-Lists", "No hay suficuente memoria");
    break;
  case DYN_EMPTY:
    log_add(log, ERROR, "Dyn-Lists", "La lista esta bacia");
    break;
  case DYN_INVALID:
    log_add(log, ERROR, "Dyn-Lists", "La lista no es valida");
    break;
  case DYN_ELEMENT_INVALID:
    log_add(log, ERROR, "Dyn-Lists", "El elemento de la lista no es valido");
    break;
  case DYN_ELEMENT_NO_MATCH:
    log_add(log, ERROR, "Dyn-Lists", "No se encontro el elemento en la lista");
    break;
  }
  dyn_error = DYN_OK;
}
DynListErrors dyn_error = DYN_OK;
