cmake_minimum_required(VERSION 3.20)

project(pipeline_lab
  VERSION 0.1.0
  DESCRIPTION "Tiny compiler + 5-stage CPU pipeline simulator"
  LANGUAGES CXX)

# ---- Standard --------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# ---- Options ---------------------------------------------------------------
option(PLAB_BUILD_TESTS "Build unit tests" ON)
option(PLAB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# ---- Warning flags (applied via the plab_warnings interface target) --------
add_library(plab_warnings INTERFACE)
if(MSVC)
  target_compile_options(plab_warnings INTERFACE /W4)
  if(PLAB_WARNINGS_AS_ERRORS)
    target_compile_options(plab_warnings INTERFACE /WX)
  endif()
else()
  target_compile_options(plab_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
  if(PLAB_WARNINGS_AS_ERRORS)
    target_compile_options(plab_warnings INTERFACE -Werror)
  endif()
endif()

# ---- Libraries / executable ------------------------------------------------
add_subdirectory(src)

# ---- Tests -----------------------------------------------------------------
if(PLAB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
