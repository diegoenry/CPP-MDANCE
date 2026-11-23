cmake_minimum_required(VERSION 3.15)
project(MDance VERSION 1.0 LANGUAGES CXX)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

# Binaries handed to other people (the release archives, see
# .github/workflows/release.yml) must run on machines other than the one that
# built them. A default build tunes for the build machine's CPU, which makes a
# binary built on a CI runner die with "illegal instruction" on an older CPU
# and cannot be combined with a macOS universal (two-architecture) build.
option(MDANCE_PORTABLE
    "Build binaries to redistribute: no -march=native, and the C++ runtime linked in"
    OFF)

if(MSVC)
    # The GCC-style flags below mean nothing to MSVC, which ignores them with a
    # warning and would leave a Release build unoptimised; CMake's own MSVC
    # defaults (/O2 /Ob2 /DNDEBUG) are right. /permissive- for standard C++
    # (e.g. the `and`/`or`/`not` keywords), /bigobj for Eigen-heavy units.
    add_compile_options(/permissive- /bigobj)
    if(MDANCE_PORTABLE)
        # The static CRT, so the .exe does not need the Visual C++
        # Redistributable installed. tests/CMakeLists.txt builds GoogleTest to match.
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0" CACHE STRING "Debug flags" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "Release flags" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG" CACHE STRING "RelWithDebInfo flags" FORCE)
    if(NOT MDANCE_PORTABLE)
        add_compile_options($<$<CONFIG:Release>:-march=native>)
    elseif(NOT APPLE)
        # libstdc++ and libgcc linked in: the binary then needs only the C
        # library (glibc) of the system it runs on. macOS's libc++ is the OS's.
        add_link_options(-static-libstdc++ -static-libgcc)
    endif()
endif()

# Enable testing at the top level so `ctest` run from the build root discovers
# the tests registered in subdirectories (must precede add_subdirectory).
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

# eQUAL / PRIME / frame-selection validation. It drives the built CLI and
# cross-checks against an independent NumPy reference, so it is registered here
# rather than in tests/, where the mdance-cli target does not exist yet.
if(BUILD_CLI)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        # The validator imports numpy. Registering it unconditionally would turn
        # a missing optional dependency into a FAILING test rather than an absent
        # one, which reads as "the code is broken".
        execute_process(COMMAND ${Python3_EXECUTABLE} -c "import numpy"
                        RESULT_VARIABLE _mdance_numpy_rc
                        OUTPUT_QUIET ERROR_QUIET)
        if(_mdance_numpy_rc EQUAL 0)
            add_test(NAME validate_equal_prime
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/validate_equal_prime.py)
            set_tests_properties(validate_equal_prime PROPERTIES
                ENVIRONMENT "MDANCE_CLI=$<TARGET_FILE:mdance-cli>")
        else()
            message(STATUS
                "numpy not importable by ${Python3_EXECUTABLE}; skipping the "
                "validate_equal_prime test (pip install numpy to enable it).")
        endif()
    endif()
endif()