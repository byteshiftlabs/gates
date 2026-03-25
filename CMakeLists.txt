cmake_minimum_required(VERSION 3.14)
project(gates VERSION 0.2.0 LANGUAGES C CXX)

# If a Conan toolchain file was injected at the super-project level but is missing pieces,
# allow an opt-out for this standalone build by setting SKIP_PARENT_TOOLCHAIN.
if(DEFINED SKIP_PARENT_TOOLCHAIN AND SKIP_PARENT_TOOLCHAIN)
  unset(CMAKE_TOOLCHAIN_FILE CACHE)
endif()

option(DEBUG "Enable debug features" OFF)
list(APPEND CMAKE_ARGS_C -DDEBUG=${DEBUG})

# Configurable limits (can be overridden via cmake -DGATES_VHDL_BIT_WIDTH=64 ..)
set(GATES_VHDL_BIT_WIDTH 32 CACHE STRING "Bit width for VHDL integer types (8-128)")
set(GATES_MAX_PARAMETERS 128 CACHE STRING "Maximum function parameters (1-1024)")
set(GATES_MAX_STRUCTS 64 CACHE STRING "Maximum struct definitions")
set(GATES_MAX_ARRAYS 128 CACHE STRING "Maximum tracked arrays")

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Collect all C sources
# List sources explicitly (begin modular refactor) instead of broad glob
set(GATES_ALL_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/astnode.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse_expression.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse_struct.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse_function.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse_statement.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse_control_flow.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/parse_for.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_constants.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_helpers.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_emit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_types.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_expressions.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen/codegen_vhdl_statements.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/token.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/utils.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/symbols/symbol_structs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/symbols/symbol_arrays.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/error_handler.c
)

# Separate main file to allow creating a reusable core library for tests
set(GATES_MAIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/app/gates.c")
set(gates_gtest_SRC ${GATES_ALL_SRC})
list(REMOVE_ITEM gates_gtest_SRC "${GATES_MAIN_SRC}")

add_library(gates_gtest ${gates_gtest_SRC})
target_include_directories(gates_gtest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(gates_gtest PUBLIC
  GATES_VHDL_BIT_WIDTH=${GATES_VHDL_BIT_WIDTH}
  GATES_MAX_PARAMETERS=${GATES_MAX_PARAMETERS}
  GATES_MAX_STRUCTS=${GATES_MAX_STRUCTS}
  GATES_MAX_ARRAYS=${GATES_MAX_ARRAYS}
)

add_executable(gates ${GATES_MAIN_SRC})
target_link_libraries(gates PRIVATE gates_gtest)
target_compile_definitions(gates PRIVATE GATES_VERSION="${PROJECT_VERSION}")

# Add compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(gates_gtest PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
  target_compile_options(gates PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

if(DEBUG)
  message(STATUS "DEBUG enabled")
  target_compile_definitions(gates_gtest PUBLIC DEBUG)
  target_compile_definitions(gates PRIVATE DEBUG)
endif()

# Install targets
install(TARGETS gates RUNTIME DESTINATION bin)

# =====================
# Static Analysis (cppcheck)
# =====================
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  message(STATUS "Found cppcheck: ${CPPCHECK}")
  add_custom_target(cppcheck
    COMMAND ${CPPCHECK}
      --enable=all
      --std=c11
      --inconclusive
      --force
      --error-exitcode=1
      --suppressions-list=${CMAKE_SOURCE_DIR}/tools/cppcheck_suppressions.txt
      -I ${CMAKE_SOURCE_DIR}/include
      -I ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/src/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck static analysis"
    VERBATIM
  )
else()
  message(STATUS "cppcheck not found — install it for static analysis (apt install cppcheck)")
endif()

# Docs target for Sphinx documentation
add_custom_target(docs
    COMMAND sphinx-build -b html docs/source docs/build/html
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building HTML documentation with Sphinx"
)

# =====================
# Testing (GoogleTest)
# =====================
option(ENABLE_TESTING "Build tests" ON)
if(ENABLE_TESTING)
  enable_testing()
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  # Try find_package first (Conan or system may provide GTest / gtest_main)
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    message(STATUS "GTest not found via package manager; fetching v1.14.0")
    FetchContent_Declare(
      googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  else()
    message(STATUS "Using existing GTest package")
  endif()

  # Collect test sources from unit and integration subdirectories
  file(GLOB_RECURSE TEST_SOURCES 
    "tests/unit/*.cpp" "tests/unit/*.cc" "tests/unit/*.cxx"
    "tests/integration/*.cpp" "tests/integration/*.cc" "tests/integration/*.cxx"
  )
  if(TEST_SOURCES)
    add_executable(gates_tests ${TEST_SOURCES})
    if(TARGET GTest::gtest_main)
      target_link_libraries(gates_tests PRIVATE GTest::gtest_main gates_gtest)
    elseif(TARGET gtest_main)
      target_link_libraries(gates_tests PRIVATE gtest_main gates_gtest)
    elseif(TARGET gtest)
      target_link_libraries(gates_tests PRIVATE gtest gates_gtest)
    else()
      message(FATAL_ERROR "No gtest_main or gtest target available")
    endif()
    target_include_directories(gates_tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    # Discover each GoogleTest test case so they show individually in CTest
    include(GoogleTest)
    gtest_discover_tests(
      gates_tests
      DISCOVERY_TIMEOUT 30
      # Optionally set WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Convenience aggregate target: build tests then run them with verbose failure output
    add_custom_target(test_all
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      DEPENDS gates_tests
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Building and running all GoogleTest cases"
    )
  endif()
endif()