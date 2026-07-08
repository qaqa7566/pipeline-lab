# Module libraries. Each add_subdirectory defines one plab_* static library.
# Dependency order: isa <- asm, isa <- sim <- viz <- cli.
add_subdirectory(isa)
add_subdirectory(asm)
add_subdirectory(sim)
add_subdirectory(viz)

# ---- CLI executable --------------------------------------------------------
add_executable(plab_cli main.cpp)
set_target_properties(plab_cli PROPERTIES OUTPUT_NAME plab)
target_link_libraries(plab_cli PRIVATE plab_viz plab_sim plab_asm plab_isa plab_warnings)
