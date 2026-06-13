cmake_minimum_required(VERSION 3.15)
project(MDance VERSION 1.0 LANGUAGES CXX)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
endif()

# Compiler flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0" CACHE STRING "Debug flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG" CACHE STRING "Release flags" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG" CACHE STRING "RelWithDebInfo flags" FORCE)

# Enable testing at the top level so `ctest` from the build root discovers tests
# registered in subdirectories (must precede the add_subdirectory calls below).
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

option(BUILD_CLI "Build mdance-cli command-line tool" ON)
if(BUILD_CLI)
    add_subdirectory(cli)
endif()

option(BUILD_SHARED "Build shared library with C API" ON)
if(BUILD_SHARED)
    add_subdirectory(capi)
endif()

option(BUILD_TCL "Build Tcl extension for VMD" OFF)
if(BUILD_TCL)
    add_subdirectory(tcl)
endif()

# eQUAL / PRIME / frame-selection validation (drives the built CLI; cross-checks
# against an independent NumPy reference). Registered here because it needs the
# mdance-cli target, which is added above.
if(BUILD_CLI)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME validate_equal_prime
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/validate_equal_prime.py)
        set_tests_properties(validate_equal_prime PROPERTIES
            ENVIRONMENT "MDANCE_CLI=$<TARGET_FILE:mdance-cli>")
    endif()
endif()