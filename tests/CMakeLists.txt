include(FetchContent)

# Pin GoogleTest for reproducible builds.
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2)

# Don't let GoogleTest override our compiler/linker settings on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# add_plab_test(<name> <source>...) links a gtest executable against the
# project libraries and registers it with CTest.
# All project libraries a test may need. plab_viz transitively brings sim+asm+isa.
set(PLAB_TEST_LIBS plab_viz)

function(add_plab_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE ${PLAB_TEST_LIBS} GTest::gtest_main)
  gtest_discover_tests(${name})
endfunction()

add_plab_test(test_encoding test_encoding.cpp)
add_plab_test(test_lexer_parser test_lexer_parser.cpp)
add_plab_test(test_assembler test_assembler.cpp)
add_plab_test(test_interpreter test_interpreter.cpp)
add_plab_test(test_pipeline_correctness test_pipeline_correctness.cpp)
add_plab_test(test_hazards test_hazards.cpp)
add_plab_test(test_viz test_viz.cpp)
