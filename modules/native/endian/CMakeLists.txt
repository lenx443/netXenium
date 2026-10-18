add_library(xen_libendian endian.c)

set_target_properties(xen_libendian PROPERTIES OUTPUT_NAME "endian")

target_link_options(xen_libendian PRIVATE
  -nodefaultlibs
)

target_include_directories(xen_libendian PRIVATE
  ${NETXENIUM_ABI_INCLUDE_PATH}
)

target_link_libraries(xen_libendian PRIVATE
  libntxenium
)

install(TARGETS xen_libendian LIBRARY DESTINATION ${NETXENIUM_MODULES_INSTALL_PATH})
