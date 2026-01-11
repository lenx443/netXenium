#include <assert.h>
#include <string.h>

#include "xen_life.h"

void test_number_run(void);
void test_string_run(void);
void test_map_run(void);
void test_boolean_run(void);
void test_modules_run(void);
void test_vector_run(void);
void test_tuple_run(void);

int main(int argc, char** argv) {
  assert(argc > 1);
  Xen_Init(argc, argv);
  const char* op = argv[1];
  if (strcmp(op, "NUMBER") == 0)
    test_number_run();
  if (strcmp(op, "STRING") == 0)
    test_string_run();
  if (strcmp(op, "MAP") == 0)
    test_map_run();
  if (strcmp(op, "BOOLEAN") == 0)
    test_boolean_run();
  if (strcmp(op, "MODULE") == 0)
    test_modules_run();
  if (strcmp(op, "VECTOR") == 0)
    test_vector_run();
  if (strcmp(op, "TUPLE") == 0)
    test_tuple_run();
  Xen_Finish();
}
