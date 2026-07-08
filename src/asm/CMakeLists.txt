add_library(plab_asm
  lexer.cpp
  parser.cpp
  assembler.cpp)

target_include_directories(plab_asm PUBLIC
  ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(plab_asm
  PUBLIC plab_isa
  PRIVATE plab_warnings)

add_library(plab::asm ALIAS plab_asm)
