add_library(xen_libifaces ifaces.c)

set_target_properties(xen_libifaces PROPERTIES OUTPUT_NAME "ifaces")

target_link_options(xen_libifaces PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libifaces PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
)

target_link_libraries(xen_libifaces PRIVATE
  libntxenium
)

install(TARGETS xen_libifaces LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
