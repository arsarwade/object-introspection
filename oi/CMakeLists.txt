add_library(toml
  support/Toml.cpp
)
target_link_libraries(toml PUBLIC tomlplusplus::tomlplusplus)

add_library(drgn_utils DrgnUtils.cpp)
target_link_libraries(drgn_utils
  glog::glog

  "-L${DRGN_PATH}/.libs"
  drgn
)
add_dependencies(drgn_utils libdrgn)

add_library(symbol_service
  Descs.cpp
  SymbolService.cpp
)
target_link_libraries(symbol_service
  drgn_utils

  Boost::headers
  ${Boost_LIBRARIES}
  glog::glog

  dw
)

add_library(container_info
  ContainerInfo.cpp
)
target_link_libraries(container_info
  glog::glog
  toml
)

add_library(codegen
  CodeGen.cpp
  Features.cpp
  FuncGen.cpp
  OICodeGen.cpp
)
target_link_libraries(codegen
  container_info
  resources
  symbol_service
  type_graph

  Boost::headers
  ${Boost_LIBRARIES}
  folly_headers
  glog::glog
)

add_subdirectory(type_graph)
