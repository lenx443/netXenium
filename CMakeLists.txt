cmake_minimum_required(VERSION 3.15)

project(NetXenium)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND DEFINED ENV{PREFIX})
    set(CMAKE_INSTALL_PREFIX "$ENV{PREFIX}" CACHE PATH "Install prefix" FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG "-fno-omit-frame-pointer -g -O0 -Wall -Wextra -Wshadow -Wcast-function-type -Wstrict-prototypes -Wmissing-prototypes -Wpedantic")

set(BUILD_SHARED_LIBS ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .so .a)
set(NETXENIUM_MODULES_INSTALL_PATH lib/netxenium)

add_compile_options(
  -DXEN_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
)

if(ASAN_ENABLE)
  message(STATUS "Enabling Address Sanitizer")
  add_compile_options(-fsanitize=address,leak,undefined)
  link_libraries(-fsanitize=address,leak,undefined)
endif()

if(SHELL_BASIC)
  message(STATUS "Enabling Shell Basic Mode")
  add_compile_options(-DSHELL_BASIC)
endif()

set(NETXENIUM_DEAFULT_INCLUDES
  core/garbage_collector/include/
  core/garbage_collector/types/header/
  core/include/
  core/common/header/
  core/common/include/
  core/common/instances/basic/include/
  core/common/instances/basic/basic_builder/include/
  core/common/instances/basic/basic_builder/types/header
  core/common/instances/function/include/
  core/common/instances/function/types/header/
  core/common/instances/string/include/
  core/common/instances/string/types/header/
  core/common/instances/number/include/
  core/common/instances/number/types/header/
  core/common/instances/vector/include/
  core/common/instances/vector/type/header/
  core/common/instances/vector/vector_iterator/include/
  core/common/instances/vector/vector_iterator/types/header/
  core/common/instances/nil/include/
  core/common/instances/map/include/
  core/common/instances/map/types/header/
  core/common/instances/boolean/include/
  core/common/instances/boolean/types/header/
  core/common/instances/tuple/include/
  core/common/instances/tuple/types/header/
  core/common/instances/tuple/tuple_iterator/include/
  core/common/instances/tuple/tuple_iterator/types/header/
  core/common/instances/method/include/
  core/common/instances/method/types/header/
  core/common/instances/except/include/
  core/common/instances/except/types/header/
  core/interpreter/include/
  core/interpreter/instances/ast/include/
  core/interpreter/instances/ast/types/header/
  core/interpreter/dynamic/vectors/include/
  core/interpreter/dynamic/trees/include/
  core/interpreter/parser/include/
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
  core/instances/igc/include/
  core/register_enginer/include/
  core/register_enginer/types/header/
  core/modules_enginer/include/
  core/modules_enginer/types/header/
  core/modules_enginer/instance/include/
  core/modules_enginer/instance/types/header/
  modules/core/include/
  dynamic/lists/include/
  dynamic/hash_tables/include/
  header/
)

set(NETXENIUM_ABI_INCLUDE_PATH
  ${CMAKE_SOURCE_DIR}/abi/include/
)

set(SOURCES
  # Codigo fuente del nucleo
  # del proyecto.
  core/src/xen_life.c
  core/src/program.c
  core/src/read_string_utf8.c

  # Recolector De Basura
  core/garbage_collector/src/xen_gc.c

  # Codigo Commun
  core/common/src/xen_alloc.c
  core/common/src/xen_cstrings.c
  core/common/src/operators.c
  core/common/src/callable.c
  core/common/src/attrs.c

  # Implementacion del tipo `Basic`
  core/common/instances/basic/src/basic.c
  core/common/instances/basic/src/basic_templates.c

  # Implementacion del tipo `BasicBuilder`
  core/common/instances/basic/basic_builder/src/basic_builder_implement.c

  # Implementacion del tipo `Function`
  core/common/instances/function/src/xen_function_implement.c
  core/common/instances/function/src/xen_function.c

  # Implementacion del tipo `String`
  core/common/instances/string/src/xen_string_implement.c
  core/common/instances/string/src/xen_string.c

  # Implementacion del tipo `Number`
  core/common/instances/number/src/xen_number_implement.c
  core/common/instances/number/src/xen_number.c

  # Implementacion del tipo `Vector`
  core/common/instances/vector/src/xen_vector_implement.c
  core/common/instances/vector/src/xen_vector.c

  # Implementacion del tipo `VectorIterator`
  core/common/instances/vector/vector_iterator/src/xen_vector_iterator_implement.c
  core/common/instances/vector/vector_iterator/src/xen_vector_iterator.c

  # Implementacion del tipo `Nil`
  core/common/instances/nil/src/xen_nil_implement.c
  core/common/instances/nil/src/xen_nil.c

  # Implementacion del tipo `Map`
  core/common/instances/map/src/xen_map_implement.c
  core/common/instances/map/src/xen_map.c

  # Implementacion del tipo `Boolean`
  core/common/instances/boolean/src/xen_boolean_implement.c
  core/common/instances/boolean/src/xen_boolean.c

  # Implementacion del tipo `Tuple`
  core/common/instances/tuple/src/xen_tuple_implement.c
  core/common/instances/tuple/src/xen_tuple.c

  # Implementacion del tipo `TupleIterator`
  core/common/instances/tuple/tuple_iterator/src/xen_tuple_iterator_implement.c
  core/common/instances/tuple/tuple_iterator/src/xen_tuple_iterator.c

  # implementacion del tipo `method`
  core/common/instances/method/src/xen_method_implement.c
  core/common/instances/method/src/xen_method.c

  # implementacion del tipo `Except`
  core/common/instances/except/src/xen_except_implement.c
  core/common/instances/except/src/xen_except.c

  # Implementacion del tipo `AST`
  core/interpreter/instances/ast/src/xen_ast.c
  core/interpreter/instances/ast/src/xen_ast_implement.c

  # Codigo que se encarga de procesar
  # e interpretar scripts
  core/interpreter/src/interpreter.c

  # Codige del Parser
  core/interpreter/parser/src/lexer.c
  core/interpreter/parser/src/parser.c
  core/interpreter/parser/src/source_file.c

  # Compilador a bytecode.
  # [AST Tree]
  #    |
  # [IR bytecode]
  #    |
  # [bytecode]
  core/interpreter/compiler/src/compiler.c
  core/interpreter/compiler/src/compiler_ext.c

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
  core/virtual_machine/types/src/vm_instructs.c
  core/virtual_machine/types/src/vm_stack.c

  # Instancias de la maquina virtual
  core/virtual_machine/instances/src/run_ctx.c
  core/virtual_machine/instances/src/run_frame.c

  # Implementaciones de los codigos
  # fuente de los vectores de la
  # maquina virtual
  core/virtual_machine/dynamic/vectors/src/bytecode.c
  core/virtual_machine/dynamic/vectors/src/vm_consts.c
  core/virtual_machine/dynamic/vectors/src/vm_backtrace.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  # de la maquina virtual
  core/virtual_machine/dynamic/lists/src/run_ctx_stack.c
  core/virtual_machine/dynamic/lists/src/vm_catch_stack.c

  # Codigo de Instancias
  core/instances/src/instance_life.c

  # Recolector de Basura de Instancias
  core/instances/igc/src/xen_igc.c

  # Implementacion de estaructura `Instance`
  core/instances/types/src/instance.c
  core/instances/types/src/implement.c

  # motor de registros
  core/register_enginer/src/xen_register.c

  # motor de modulos
  core/modules_enginer/src/xen_modules_def.c
  core/modules_enginer/src/xen_module_load.c

  # Instanca del motor de modulos
  core/modules_enginer/instance/src/xen_module_implement.c
  core/modules_enginer/instance/src/xen_module.c

  # modulos
  modules/core/src/m_core.c
  modules/core/src/m_interpreter.c
  modules/core/src/m_config.c

  # Implementaciones de los codigos
  # fuente de las listas enlazadas
  dynamic/lists/src/list.c
  dynamic/lists/src/history.c
  dynamic/lists/src/string_utf8.c
)

install(DIRECTORY ${NETXENIUM_ABI_INCLUDE_PATH} DESTINATION include)

add_library(libntxenium
  abi/src/abi.c
  ${SOURCES}
)
target_include_directories(libntxenium PRIVATE ${NETXENIUM_DEAFULT_INCLUDES} ${NETXENIUM_ABI_INCLUDE_PATH})

set_target_properties(libntxenium PROPERTIES OUTPUT_NAME "ntxenium")

target_compile_options(libntxenium PRIVATE -DXEN_ABI)

install(TARGETS libntxenium LIBRARY DESTINATION lib)

add_executable(ntxenium
  # Codigo fuente principal (Funcion main)
  main.c
  ${SOURCES}
)

target_include_directories(ntxenium PRIVATE ${NETXENIUM_DEAFULT_INCLUDES})

target_compile_options(ntxenium PRIVATE -DXEN_CORE)

install(TARGETS ntxenium RUNTIME DESTINATION bin)

add_subdirectory(modules/native/io)
add_subdirectory(modules/native/sockets)

enable_testing()

add_executable(tester
  ${SOURCES}
  tests/test_number.c
  tests/test_string.c
  tests/test_map.c
  tests/test_boolean.c
  tests/test_modules.c
  tests/test_load_script.c
  tests/test_vector.c
  tests/test_tuple.c
  tests/test_main.c
)

target_include_directories(tester PRIVATE ${NETXENIUM_DEAFULT_INCLUDES})

target_compile_options(tester PRIVATE -DXEN_TEST)

add_test(NAME TestNumber      COMMAND tester      "NUMBER")
add_test(NAME TestString      COMMAND tester      "STRING")
add_test(NAME TestMap         COMMAND tester      "MAP")
add_test(NAME TestBoolean     COMMAND tester      "BOOLEAN")
add_test(NAME TestModules     COMMAND tester      "MODULES")
add_test(NAME TestLoadScript  COMMAND tester      "LOAD_SCRIPT")
add_test(NAME TestVector      COMMAND tester      "VECTOR")
add_test(NAME TestTuple       COMMAND tester      "TUPLE")
