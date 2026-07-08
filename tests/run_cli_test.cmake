# Driver for CLI-level tests: runs the `plab` binary and asserts its exit code
# (and, optionally, that its combined output does / does not contain a pattern).
# Comparing the exact exit code means a crash (SIGABRT / non-zero-by-signal)
# never masquerades as the expected clean failure code.
#
# Invoked by add_test as:
#   cmake -DPLAB=<binary> -DEXPECT_CODE=<n> [-DARGS=a|b|c]
#         [-DEXPECT_CONTAINS=<regex>] [-DEXPECT_NOT_CONTAINS=<regex>]
#         -P run_cli_test.cmake
#
# ARGS is a '|'-delimited list (avoids the ';' list-splitting pitfalls of -D).

if(NOT DEFINED PLAB)
  message(FATAL_ERROR "run_cli_test: PLAB not set")
endif()

# Turn the '|'-delimited ARGS string into a real CMake list of arguments.
set(_arglist "")
if(DEFINED ARGS AND NOT ARGS STREQUAL "")
  string(REPLACE "|" ";" _arglist "${ARGS}")
endif()

execute_process(
  COMMAND "${PLAB}" ${_arglist}
  RESULT_VARIABLE _code
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err)

set(_combined "${_out}${_err}")

if(NOT "${_code}" STREQUAL "${EXPECT_CODE}")
  message(FATAL_ERROR
    "expected exit code '${EXPECT_CODE}', got '${_code}'\n"
    "--- args ---\n${_arglist}\n--- output ---\n${_combined}")
endif()

if(EXPECT_CONTAINS AND NOT _combined MATCHES "${EXPECT_CONTAINS}")
  message(FATAL_ERROR
    "output did not match '${EXPECT_CONTAINS}'\n--- output ---\n${_combined}")
endif()

if(EXPECT_NOT_CONTAINS AND _combined MATCHES "${EXPECT_NOT_CONTAINS}")
  message(FATAL_ERROR
    "output unexpectedly matched '${EXPECT_NOT_CONTAINS}'\n--- output ---\n${_combined}")
endif()
