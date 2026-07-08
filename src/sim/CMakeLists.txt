add_library(plab_sim
  alu.cpp
  interpreter.cpp
  pipeline.cpp)

target_include_directories(plab_sim PUBLIC
  ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(plab_sim
  PUBLIC plab_isa plab_asm
  PRIVATE plab_warnings)

add_library(plab::sim ALIAS plab_sim)
