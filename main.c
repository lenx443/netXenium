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

#include <locale.h>
#include <signal.h>
#include <stdlib.h>

#include "list.h"
#include "logs.h"
#include "program.h"
#include "properties.h"
#include "run_ctx_stack.h"
#include "vm_def.h"

int main(int argc, char **argv) {
  global_logs = list_new();
  if (!vm_create()) {
    log_free(NULL);
    return 1;
  }
  prop_register = prop_reg_new();
  if (prop_register == NULL) {
    log_add(NULL, ERROR, argv[0], "No se pudo iniciar la configuracion");
    log_show_and_clear(NULL);
    vm_destroy();
    log_free(NULL);
    return 1;
  }
  run_context_stack_push(&vm->vm_ctx_stack);
  setlocale(LC_CTYPE, "");

  program.argv = argv;
  program.argc = argc;

  if (argc > 1) {
    program.name = strdup(argv[1]);
    load_script(argv[1]);
  } else {
    program.name = strdup(argv[0]);
    signal(SIGINT, SIG_IGN);
    shell_loop(argv[0]);
  }

  free(program.name);
  prop_reg_free(prop_register);
  vm_destroy();
  log_free(NULL);
  return program.exit_code;
}
