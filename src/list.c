#include <stdint.h>
#include <stdlib.h>

#include "error_codes.h"
#include "errs.h"
#include "list.h"

LIST_ptr list_new() {
  LIST_ptr list = malloc(sizeof(LIST));
  if (list == NULL) {
    code_error = ERROR_LIST_MEMORY;
    return NULL;
  }
  list->head = NULL;
  list->tail = NULL;
  return list;
}

LIST_ptr list_new_string(char *str) {
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
    code_error = ERROR_LIST_NULL;
    return 1;
  }
  return 0;
}

int list_push_back(LIST_ptr list, void *value, size_t size) {
  if (list == NULL) {
    code_error = ERROR_LIST_NULL;
    return 0;
  }
  NODE_ptr newNode = malloc(sizeof(NODE));
  if (newNode == NULL) {
    code_error = ERROR_LIST_MEMORY;
    return 0;
  }
  newNode->size = size;
  newNode->point = malloc(newNode->size);
  memcpy(newNode->point, value, newNode->size);
  newNode->next = NULL;
  if (list->head == NULL)
    list->head = newNode;
  else
    list->tail->next = newNode;
  list->tail = newNode;
  return 1;
}

int list_push_back_string(LIST_ptr list, char *str) {
  for (char *p = str; *p != '\0'; p++) {
    if (!list_push_back(list, p, 1))
      return 0;
  }
  return 1;
}

int list_push_back_string_node(LIST_ptr list, char *str) {
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
    NODE *data = malloc(sizeof(NODE));
    if (data == NULL) {
      code_error = ERROR_LIST_MEMORY;
      return NULL;
    }
    data->point = malloc(current->size);
    if (data->point == NULL) {
      code_error = ERROR_LIST_MEMORY;
      free(data);
      return NULL;
    }
    memcpy(data->point, current->point, current->size);
    data->size = current->size;
    data->next = NULL;
    free(current->point);
    free(current);
    list->head = list->tail = NULL;
    return data;
  }
  while (current->next != NULL && current->next != list->tail)
    current = current->next;
  prev = current;
  current = current->next;
  NODE_ptr data = malloc(sizeof(NODE));
  if (data == NULL) {
    code_error = ERROR_LIST_MEMORY;
    return NULL;
  }
  data->point = malloc(current->size);
  if (data->point == NULL) {
    code_error = ERROR_LIST_MEMORY;
    free(data);
    return NULL;
  }
  memcpy(data->point, current->point, current->size);
  data->size = current->size;
  data->next = NULL;
  free(current->point);
  free(current);
  prev->next = NULL;
  list->tail = prev;
  return data;
}

int list_search(LIST list, void *value, size_t size) {
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
  code_error = ERROR_LIST_FINDED;
  return -1;
}

int list_forEach(NODE_ptr *current, LIST list) {
  if (current == NULL) {
    code_error = ERROR_LIST_NODE_NULL;
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
    code_error = ERROR_LIST_INDEX;
    return NULL;
  }
  NODE_ptr current = list.head;
  int n = 0;
  while (current != NULL && n < index) {
    current = current->next;
    n++;
  }
  if (current == NULL) {
    code_error = ERROR_LIST_INDEX;
    return NULL;
  }
  return current;
}

int list_index_set(int index, LIST_ptr list, void *value, size_t size) {
  if (list_empty(list)) {
    return 0;
  }
  if (index < 0) {
    code_error = ERROR_LIST_INDEX;
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
    code_error = ERROR_LIST_INDEX;
    return 0;
  }
  NODE_ptr newNode = malloc(sizeof(NODE));
  if (newNode == NULL) {
    code_error = ERROR_LIST_MEMORY;
    return 0;
  }
  newNode->point = malloc(size);
  if (newNode->point == NULL) {
    code_error = ERROR_LIST_MEMORY;
    free(newNode);
    return 0;
  }
  memcpy(newNode->point, value, size);
  newNode->size = size;
  newNode->next = current->next;
  if (prev == NULL)
    list->head = newNode;
  else
    prev->next = newNode;
  free(current->point);
  free(current);
  return 1;
}

void list_as_string(LIST list, char *str, int size) {
  int n = 0;
  NODE_ptr node = NULL;
  FOR_EACH(&node, list) {
    if (n < size - 1)
      str[n++] = *(char *)node->point;
    else
      break;
  }
  str[n] = '\0';
}

void list_free(LIST_ptr list) {
  if (list_empty(list)) {
    return;
  }
  NODE_ptr current = list->head;
  while (current != NULL) {
    NODE_ptr next = current->next;
    free(current->point);
    free(current);
    current = next;
  }
  free(list);
}

int node_empty(NODE_ptr *node) {
  if (node == NULL || *node == NULL) {
    code_error = ERROR_LIST_NODE_NULL;
    return 1;
  }
  return 0;
}

void node_free(NODE_ptr *node) {
  if (node_empty(node)) {
    return;
  }
  free((*node)->point);
  free(*node);
  *node = NULL;
}
