add_library(plab_isa
  instruction.cpp
  encoding.cpp)

target_include_directories(plab_isa PUBLIC
  ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(plab_isa PRIVATE plab_warnings)

add_library(plab::isa ALIAS plab_isa)
