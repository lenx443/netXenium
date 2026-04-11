add_library(xen_libos os.c)

set_target_properties(xen_libos PROPERTIES OUTPUT_NAME "os")

target_link_options(xen_libos PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libos PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
)

target_link_libraries(xen_libos PRIVATE
  libntxenium
)

install(TARGETS xen_libos LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
