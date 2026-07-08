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

# ---- CLI-level tests -------------------------------------------------------
# Drive the built `plab` binary end-to-end and assert its exit code / output.
# add_cli_test(<name> EXPECT_CODE <n> [ARGS a b ...]
#              [CONTAINS <regex>] [NOT_CONTAINS <regex>])
set(CLI_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_test.cmake)
set(SIMPLE_ADD ${CMAKE_SOURCE_DIR}/examples/simple_add.asm)

function(add_cli_test name)
  cmake_parse_arguments(T "" "EXPECT_CODE;CONTAINS;NOT_CONTAINS" "ARGS" ${ARGN})
  list(JOIN T_ARGS "|" joined_args)  # '|'-delimited: dodges ';' list splitting
  add_test(
    NAME cli_${name}
    COMMAND ${CMAKE_COMMAND}
      -DPLAB=$<TARGET_FILE:plab_cli>
      -DEXPECT_CODE=${T_EXPECT_CODE}
      -DARGS=${joined_args}
      -DEXPECT_CONTAINS=${T_CONTAINS}
      -DEXPECT_NOT_CONTAINS=${T_NOT_CONTAINS}
      -P ${CLI_DRIVER})
endfunction()

# Happy path: a valid program runs to completion.
add_cli_test(simple_add_ok        EXPECT_CODE 0 ARGS ${SIMPLE_ADD}
             CONTAINS "Performance Report")
# Valid --mem still works (and combines with other flags).
add_cli_test(mem_valid            EXPECT_CODE 0
             ARGS ${SIMPLE_ADD} --mem 4 --interp --stats
             CONTAINS "Data memory")
# Invalid / overflowing --mem must fail cleanly, never SIGABRT.
add_cli_test(mem_invalid          EXPECT_CODE 2 ARGS ${SIMPLE_ADD} --mem abc
             CONTAINS "error:" NOT_CONTAINS "terminating")
add_cli_test(mem_huge             EXPECT_CODE 2 ARGS ${SIMPLE_ADD} --mem 99999999999
             CONTAINS "error:" NOT_CONTAINS "terminating")
add_cli_test(mem_missing_value    EXPECT_CODE 2 ARGS ${SIMPLE_ADD} --mem
             CONTAINS "error:" NOT_CONTAINS "terminating")
# Other CLI error paths.
add_cli_test(missing_file         EXPECT_CODE 2 ARGS does_not_exist.asm
             CONTAINS "error:")
add_cli_test(unknown_option       EXPECT_CODE 2 ARGS ${SIMPLE_ADD} --bogus
             CONTAINS "error:")
