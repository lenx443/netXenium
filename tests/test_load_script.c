#include <assert.h>

#include "program.h"
#include "xen_life.h"

int main(int argc, char **argv) {
  assert(Xen_Init(argc, argv));
  load_script("./script.nxm");
  Xen_Finish();
}
