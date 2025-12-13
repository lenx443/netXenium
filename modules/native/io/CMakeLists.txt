add_library(xen_libio io.c)

set_target_properties(xen_libio PROPERTIES OUTPUT_NAME "io")

target_compile_options(xen_libio PRIVATE
  -ffreestanding
  -fno-builtin
  -nostdlib
)

target_link_options(xen_libio PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libio PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
)

target_link_libraries(xen_libio PRIVATE
  libntxenium
)

install(TARGETS xen_libio LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
