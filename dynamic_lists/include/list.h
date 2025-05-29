#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

#define FOR_EACH(node, list) while (list_forEach(node, list))

typedef enum {
  DYN_OK = 0,
  DYN_NO_MEMORY,
  DYN_EMPTY,
  DYN_INVALID,
  DYN_ELEMENT_INVALID,
  DYN_ELEMENT_NO_MATCH,
} DynListErrors;

struct Node {
  void *point;
  size_t size;
  struct Node *next;
};

struct List {
  struct Node *head;
  struct Node *tail;
};

typedef struct Node NODE;
typedef struct List LIST;
typedef NODE *NODE_ptr;
typedef LIST *LIST_ptr;

LIST_ptr list_new();
LIST_ptr list_new_string(char *);
int list_size(LIST);
int list_empty(LIST_ptr);
int list_valid(LIST_ptr);
int list_push_back(LIST_ptr, void *, size_t);
int list_push_begin(LIST_ptr, void *, size_t);
int list_push_back_string(LIST_ptr, char *);
int list_push_back_string_node(LIST_ptr, char *);
NODE_ptr list_pop_back(LIST_ptr);
void list_erase_at_index(LIST_ptr, int);
int list_search(LIST, void *, size_t);
int list_search_string(LIST, char *);
int list_forEach(NODE_ptr *, LIST);
NODE_ptr list_index_get(int, LIST);
int list_index_set(int, LIST_ptr, void *, size_t);
void list_as_string(LIST, char *, int);
void list_clear(LIST_ptr);
void list_free(LIST_ptr);

int node_empty(NODE_ptr *);
void node_free(NODE_ptr *);

void DynSetLog(LIST_ptr);
extern DynListErrors dyn_error;

#endif
