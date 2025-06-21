#ifndef __STRING_HASH_TABLE_H__
#define __STRING_HASH_TABLE_H__

struct String_Hash_Node {
  char *h_key;
  void *h_value;
  int h_value_size;
  struct String_Hash_Node *next_hash_node;
};

struct String_Hash_Table {
  struct String_Hash_Node **ht_bucket;
  int ht_capacity;
};

typedef struct String_Hash_Node Str_HT_Node;
typedef struct String_Hash_Table Str_HT;
typedef Str_HT_Node *Str_HT_Node_ptr;
typedef Str_HT *Str_HT_ptr;

// djb2 hash function
unsigned long str_hash(const char *);
Str_HT_ptr str_ht_new(int);
int str_ht_insert(Str_HT_ptr, const char *, void *, int);
int str_ht_delete(Str_HT_ptr, const char *);
Str_HT_Node *str_ht_get(Str_HT_ptr, const char *);
void str_ht_free(Str_HT_ptr);

#endif
