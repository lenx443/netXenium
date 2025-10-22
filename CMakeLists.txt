cmake_minimum_required(VERSION 3.15)

project(NetXenium)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_DEBUG "-fsanitize=address,leak -fno-omit-frame-pointer -g -O0 -Wall -Wextra -Wshadow")

set(BUILD_SHARED_LIBS ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .so .a)

set(C_STANDARD 99)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Enabling Address Sanitizer")
  link_libraries(-fsanitize=address,undefined)
endif()

include_directories(
  core/include/
  core/common/include/
  core/common/instances/basic/include/
  core/common/instances/command/include/
  core/common/instances/command/types/header/
  core/common/instances/string/include/
  core/common/instances/string/types/header/
  core/common/instances/number/include/
  core/common/instances/number/types/header/
  core/common/instances/vector/include/
  core/common/instances/vector/type/header/
  core/common/instances/nil/include/
  core/common/instances/map/include/
  core/common/instances/map/types/header/
  core/common/instances/boolean/include/
  core/common/instances/boolean/types/header/
  core/common/instances/tuple/include/
  core/common/instances/tuple/types/header/
  core/interpreter/include/
  core/interpreter/instances/ast/include/
  core/interpreter/instances/ast/types/header/
  core/interpreter/dynamic/vectors/include/
  core/interpreter/dynamic/trees/include/
  core/interpreter/compiler/include/
  core/interpreter/compiler/types/header/
  core/interpreter/compiler/dynamic/lists/include/
  core/interpreter/compiler/dynamic/vectors/include/
  core/virtual_machine/include/
  core/virtual_machine/types/include/
  core/virtual_machine/instances/include/
  core/virtual_machine/instances/types/header/
  core/virtual_machine/dynamic/vectors/include/
  core/virtual_machine/dynamic/lists/include/
  core/instances/include
  core/instances/types/include/
  core/register_enginer/include/
  core/register_enginer/types/header/
  core/modules_enginer/include/
  core/modules_enginer/types/header/
  core/modules_enginer/instance/include/
  core/modules_enginer/instance/types/header/
  modules/include/
  dynamic/lists/include/
  dynamic/hash_tables/include/
  header/
)

set(SOURCES
  # Codigo fuente donde del nucleo
  # del proyecto.
  core/src/xen_life.c
  core/src/program.c
  core/src/read_string_utf8.c

  # Codigo Commun
  core/common/src/operators.c
  core/common/src/callable.c

  # Implementacion del objeto `Basic`
  core/common/instances/basic/src/basic.c

  # Implementacion del objeto `Command`
  core/common/instances/command/src/xen_command_implement.c
  core/common/instances/command/src/xen_command.c

  # Implementacion del objeto `String`
  core/common/instances/string/src/xen_string_implement.c
  core/common/instances/string/src/xen_string.c

  # Implementacion del objeto `Number`
  core/common/instances/number/src/xen_number_implement.c
  core/common/instances/number/src/xen_number.c

  # Implementacion del objeto `Vector`
  core/common/instances/vector/src/xen_vector_implement.c
  core/common/instances/vector/src/xen_vector.c

  # Implementacion del objeto `Nil`
  core/common/instances/nil/src/xen_nil_implement.c
  core/common/instances/nil/src/xen_nil.c

  # Implementacion del objeto `Map`
  core/common/instances/map/src/xen_map_implement.c
  core/common/instances/map/src/xen_map.c

  # Implementacion del objeto `Boolean`
  core/common/instances/boolean/src/xen_boolean_implement.c
  core/common/instances/boolean/src/xen_boolean.c

  # Implementacion del objeto `Tuple`
  core/common/instances/tuple/src/xen_tuple_implement.c
  core/common/instances/tuple/src/xen_tuple.c

  # Implementacion del objeto `AST`
  core/interpreter/instances/ast/src/xen_ast.c
  core/interpreter/instances/ast/src/xen_ast_implement.c

  # Codigo que se encarga de procesar
  # e interpretar scripts
  core/interpreter/src/interpreter.c
  core/interpreter/src/lexer.c
  core/interpreter/src/parser.c

  # Compilador a bytecode.
  # [AST Tree]
  #    |
  # [IR bytecode]
  #    |
  # [bytecode]
  core/interpreter/compiler/src/ast_compiler.c
  core/interpreter/compiler/src/blocks_linealizer.c
  core/interpreter/compiler/src/blocks_compiler.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  # de el compilador
  core/interpreter/compiler/dynamic/lists/src/block_list.c

  # Implementaciones de los codigos
  # fuente de los vectores de el
  # compilador
  core/interpreter/compiler/dynamic/vectors/src/ir_bytecode.c

  # Entorno de maquina virtual para la
  # ejecucion de bytecode.
  core/virtual_machine/src/vm.c
  core/virtual_machine/src/vm_run.c

  # Tipos definidos en la maquina virtual
  core/virtual_machine/types/src/vm_def.c
  core/virtual_machine/types/src/vm_register.c
  core/virtual_machine/types/src/vm_instructs.c

  # Instancias de la maquina virtual
  core/virtual_machine/instances/src/run_ctx.c
  core/virtual_machine/instances/src/run_frame.c

  # Implementaciones de los codigos
  # fuente de los vectores de la
  # maquina virtual
  core/virtual_machine/dynamic/vectors/src/bytecode.c
  core/virtual_machine/dynamic/vectors/src/vm_consts.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  # de la maquina virtual
  core/virtual_machine/dynamic/lists/src/run_ctx_stack.c

  # Codigo de Instancias
  core/instances/src/instance_life.c

  # Implementacion de estaructura `Instance`
  core/instances/types/src/implement.c
  core/instances/types/src/instance.c

  # motor de registros
  core/register_enginer/src/xen_register.c

  # motor de modulos
  core/modules_enginer/src/xen_modules_def.c
  core/modules_enginer/src/xen_module_load.c

  # Instanca del motor de modulos
  core/modules_enginer/instance/src/xen_module_implement.c
  core/modules_enginer/instance/src/xen_module.c

  # modulos
  modules/src/m_core.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  dynamic/lists/src/list.c
  dynamic/lists/src/history.c
  dynamic/lists/src/suggestion.c
  dynamic/lists/src/logs.c
  dynamic/lists/src/string_utf8.c
)

add_executable(ntxenium
  # Codigo fuente principal (Funcion main)
  main.c
  ${SOURCES}
)

enable_testing()

add_executable(test_number
  tests/test_number.c ${SOURCES}
)
add_test(NAME TestNumber COMMAND test_number)

add_executable(test_string
  tests/test_string.c ${SOURCES}
)
add_test(NAME TestString COMMAND test_string)

add_executable(test_map
  tests/test_map.c ${SOURCES}
)
add_test(NAME TestMap COMMAND test_map)

add_executable(test_boolean
  tests/test_boolean.c ${SOURCES}
)
add_test(NAME TestBoolean COMMAND test_boolean)

add_executable(test_modules
  tests/test_modules.c ${SOURCES}
)
add_test(NAME TestModules COMMAND test_modules)

add_executable(test_load_script
  tests/test_load_script.c ${SOURCES}
)
add_test(NAME TestLoadScript COMMAND test_load_script)

add_executable(test_vector
  tests/test_vector.c ${SOURCES}
)
add_test(NAME TestVector COMMAND test_vector)

add_executable(test_tuple
  tests/test_tuple.c ${SOURCES}
)
add_test(NAME TestTuple COMMAND test_tuple)
