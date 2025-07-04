#include <stdlib.h>
#include <string.h>

#include "logs.h"
#include "string_hash_table.h"

uint32_t hash_fnv_1a(const void *key, size_t len) {
  const uint8_t *data = (const uint8_t *)key;
  uint32_t hash = 2166136261u;

  for (size_t i = 0; i < len; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }

  return hash;
}

unsigned long str_hash(const char *str) {
  unsigned long hash = 0x1505;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

Str_HT_ptr str_ht_new(int capacity) {
  Str_HT_ptr ht = malloc(sizeof(Str_HT));
  if (ht == NULL) {
    log_add(NULL, ERROR, "Hash Table", "No hay suficiente memoria disponible");
    return NULL;
  }
  ht->ht_bucket = calloc(capacity, sizeof(Str_HT_Node_ptr));
  if (ht->ht_bucket == NULL) {
    free(ht);
    log_add(NULL, ERROR, "Hash Table", "No hay suficiente memoria disponible");
    return NULL;
  }
  ht->ht_capacity = capacity;
  return ht;
}

int str_ht_insert(Str_HT_ptr ht, const char *key, void *value, int value_size) {
  if (!ht || !key || !value || value_size <= 0) {
    log_add(NULL, ERROR, "Hash Table", "No se pudo insertar el elemento");
    return 0;
  }
  unsigned long hash = str_hash(key);
  int ht_index = hash % ht->ht_capacity;

  Str_HT_Node_ptr current_node = ht->ht_bucket[ht_index];
  while (current_node) {
    if (strcmp(current_node->h_key, key) == 0) {
      void *new_value = malloc(value_size);
      if (!new_value) {
        log_add(NULL, ERROR, "Hash Table",
                "Memoria insuficiente para agregar el nuevo elemento");
        return 0;
      }
      memcpy(new_value, value, value_size);
      free(current_node->h_value);
      current_node->h_value = new_value;
      current_node->h_value_size = value_size;
      return 1;
    }
    current_node = current_node->next_hash_node;
  }

  Str_HT_Node_ptr new_node = malloc(sizeof(Str_HT_Node));
  if (!new_node) {
    log_add(NULL, ERROR, "Hash Table",
            "Memoria insuficiente para agregar el nuevo elemento");
    return 0;
  }
  new_node->h_key = strdup(key);
  if (!new_node->h_key) {
    log_add(NULL, ERROR, "Hash Table",
            "Memoria insuficiente para agregar el nuevo elemento");
    free(new_node);
    return 0;
  }
  new_node->h_value = malloc(value_size);
  if (!new_node->h_value) {
    log_add(NULL, ERROR, "Hash Table",
            "Memoria insuficiente para agregar el nuevo elemento");
    free(new_node->h_key);
    free(new_node);
    return 0;
  }
  memcpy(new_node->h_value, value, value_size);
  new_node->h_value_size = value_size;

  new_node->next_hash_node = ht->ht_bucket[ht_index];
  ht->ht_bucket[ht_index] = new_node;
  return 1;
}

int str_ht_delete(Str_HT_ptr ht, const char *key) {
  if (!ht || !key || ht->ht_capacity <= 0) {
    log_add(NULL, ERROR, "Hash Table", "No se pudo eliminar el elemento");
    return 0;
  }

  unsigned long hash = str_hash(key);
  int ht_index = hash % ht->ht_capacity;

  Str_HT_Node_ptr current = ht->ht_bucket[ht_index];
  Str_HT_Node_ptr previous = NULL;

  while (current) {
    if (strcmp(current->h_key, key) == 0) {
      if (previous)
        previous->next_hash_node = current->next_hash_node;
      else
        ht->ht_bucket[ht_index] = current->next_hash_node;
      free(current->h_key);
      free(current->h_value);
      free(current);
      return 1;
    }
    previous = current;
    current = current->next_hash_node;
  }
  return 0;
}

Str_HT_Node_ptr str_ht_get(Str_HT_ptr ht, const char *key) {
  if (!ht || !key || ht->ht_capacity <= 0) {
    log_add(NULL, ERROR, "Hash Table", "No se pudo obtener el valor");
    return NULL;
  }
  unsigned long hash = str_hash(key);
  int ht_index = hash % ht->ht_capacity;

  Str_HT_Node_ptr current = ht->ht_bucket[ht_index];
  while (current) {
    if (strcmp(current->h_key, key) == 0) { return current; }
    current = current->next_hash_node;
  }
  return NULL;
}

void str_ht_free(Str_HT_ptr ht) {
  if (!ht) return;
  for (int i = 0; i < ht->ht_capacity; i++) {
    Str_HT_Node_ptr current = ht->ht_bucket[i];
    while (current) {
      Str_HT_Node_ptr temp = current;
      current = current->next_hash_node;
      free(temp->h_key);
      free(temp->h_value);
      free(temp);
    }
  }
  free(ht->ht_bucket);
  free(ht);
}
