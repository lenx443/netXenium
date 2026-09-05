add_library(xen_libchecksum checksum.c)

set_target_properties(xen_libchecksum PROPERTIES OUTPUT_NAME "checksum")

target_link_options(xen_libchecksum PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libchecksum PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
)

target_link_libraries(xen_libchecksum PRIVATE
  libntxenium
)

install(TARGETS xen_libchecksum LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
