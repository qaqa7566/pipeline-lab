add_library(plab_viz
  trace.cpp)

target_include_directories(plab_viz PUBLIC
  ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(plab_viz
  PUBLIC plab_sim
  PRIVATE plab_warnings)

add_library(plab::viz ALIAS plab_viz)
