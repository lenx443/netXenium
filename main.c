/*
 * Netxenium Network Framework
 * Copyright (C) 2024-2025 Lenx443 <menasalejandro153@gmail.com>
 *
 * Este programa es software libre: usted puede redistribuirlo y/o modificarlo
 * bajo los términos de la Licencia Pública General GNU publicada por la
 * Free Software Foundation, ya sea la versión 3 de la Licencia, o
 * (a su elección) cualquier versión posterior.
 *
 * Este programa se distribuye con la esperanza de que sea útil,
 * pero SIN NINGUNA GARANTÍA; incluso sin la garantía implícita de
 * COMERCIABILIDAD o IDONEIDAD PARA UN PROPÓSITO PARTICULAR. Vea la
 * Licencia Pública General GNU para más detalles.
 *
 * Usted debería haber recibido una copia de la Licencia Pública General GNU
 * junto con este programa. Si no, véase <https://www.gnu.org/licenses/>.
 */

/*
 * Netxenium Network Framework
 * Copyright (C) 2024-2025 Lenx443 <menasalejandro153@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "program.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_life.h"
#include "xen_module.h"

int main(int argc, char** argv) {
  if (!Xen_Init(argc, argv)) {
    return 1;
  }
  if (argc > 1) {
    program.name = Xen_CString_Dup(argv[1]);
    const char* slash = strrchr(argv[1], '/');
    if (slash) {
      size_t len = slash - argv[1];
      char* dir = Xen_Alloc(len + 1);
      memcpy(dir, argv[1], len);
      dir[len] = '\0';
      Xen_Module_Load(argv[1], "<start>", dir, vm->globals_instances,
                      XEN_MODULE_GUEST);
    } else {
      Xen_Module_Load(argv[1], "<start>", ".", vm->globals_instances,
                      XEN_MODULE_GUEST);
    }
  } else {
    program.name = Xen_CString_Dup(argv[0]);
    shell_loop();
  }
  Xen_Finish();
  return program.exit_code;
}
