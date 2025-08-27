cmake_minimum_required(VERSION 3.15)

project(NetXenium)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

set(BUILD_SHARED_LIBS ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .so .a)

set(C_STANDARD 99)

include_directories(
  core/include/
  core/interpreter/include/
  core/interpreter/dynamic/vectors/include/
  core/interpreter/dynamic/trees/include/
  core/interpreter/compiler/include/
  core/interpreter/compiler/types/header/
  core/interpreter/compiler/dynamic/lists/include/
  core/interpreter/compiler/dynamic/vectors/include/
  core/virtual_machine/include/
  core/virtual_machine/types/header/
  core/virtual_machine/types/include/
  core/virtual_machine/dynamic/vectors/include/
  core/virtual_machine/dynamic/lists/include/
  core/instances/types/include/
  core/instances/dynamic/vectors/include/
  core/instances/callable/types/include/
  core/instances/callable/dynamic/vectors/include/
  core/instances/basic/include/
  core/instances/command/include/
  core/instances/command/types/header/
  core/instances/string/include/
  core/instances/string/types/header/
  core/instances/number/include/
  core/instances/number/types/header/
  core/register_enginer/include/
  core/register_enginer/types/header/
  api/include/
  dynamic/lists/include/
  dynamic/hash_tables/include/
  header/
)

add_executable(ntxenium
  # Codigo fuente principal (Funcion main)
  main.c

  # Codigo fuente donde del nucleo
  # del proyecto.
  core/src/xen_life.c
  core/src/properties_types.c
  core/src/program.c
  core/src/read_string_utf8.c

  # Codigo que se encarga de procesar
  # e interpretar scripts
  core/interpreter/src/interpreter.c
  core/interpreter/src/lexer.c
  core/interpreter/src/parser.c

  # Implementaciones de los codigos
  # fuente de los arboles dinamicos
  # del interpreter
  core/interpreter/dynamic/trees/src/ast.c

  # Implementaciones de los codigos
  # fuente de los vectores del
  # interpreter
  core/interpreter/dynamic/vectors/src/ast_array.c

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
  core/virtual_machine/src/garbage_collector.c

  # Tipos definidos en la maquina virtual
  core/virtual_machine/types/src/GCPointer.c
  core/virtual_machine/types/src/vm_def.c
  core/virtual_machine/types/src/vm_register.c
  core/virtual_machine/types/src/run_frame.c

  # Implementaciones de los codigos
  # fuente de los vectores de la
  # maquina virtual
  core/virtual_machine/dynamic/vectors/src/bytecode.c
  core/virtual_machine/dynamic/vectors/src/vm_consts.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  # de la maquina virtual
  core/virtual_machine/dynamic/lists/src/gc_pointer_list.c
  core/virtual_machine/dynamic/lists/src/run_ctx_stack.c

  # Implementaciones de los codigos
  # fuente de los vectores de los objetos
  # Callables
  core/instances/callable/dynamic/vectors/src/call_args.c

  # Implementacion de estructura `Callable`
  core/instances/callable/types/src/callable.c

  # Implementacion de estaructura `Instance`
  core/instances/types/src/implement.c
  core/instances/types/src/instance.c

  # Implementacion del objeto `Basic`
  core/instances/basic/src/basic.c

  # Implementacion del objeto `Command`
  core/instances/command/src/xen_command_implement.c
  core/instances/command/src/xen_command.c

  # Implementaciones de los codigos
  # fuente de los vectores de las
  # Instancias
  core/instances/dynamic/vectors/src/instances_map.c

  # Implementacion del objeto `String`
  core/instances/string/src/xen_string_implement.c
  core/instances/string/src/xen_string.c

  # Implementacion del objeto `Number`
  core/instances/number/src/xen_number_implement.c
  core/instances/number/src/xen_number.c

  # motor de registros
  core/register_enginer/src/xen_register.c

  # codigo fuente de la la API
  # del Framework.
  api/src/functions.c
  api/src/pcap_wrapper.c
  api/src/terminal.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  dynamic/lists/src/list.c
  dynamic/lists/src/properties.c
  dynamic/lists/src/history.c
  dynamic/lists/src/suggestion.c
  dynamic/lists/src/logs.c
  dynamic/lists/src/string_utf8.c

  # Implementaciones de los codigos
  # fuente de las tablas hash
  dynamic/hash_tables/src/string_hash_table.c

  # Implementaciones de los codigo fuente
  # de los comandos definidos en
  # core/include/commands.h
  commands/help.c
  commands/exit.c
  commands/new.c
  commands/del.c
  commands/set.c
  commands/get.c
  commands/echo.c
  commands/input.c
  commands/clear_history.c
  commands/resolve.c
  commands/arp_spoof.c
  commands/iface.c
)

target_include_directories(ntxenium PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(ntxenium PRIVATE ${PCAP_LIBRARIES})
