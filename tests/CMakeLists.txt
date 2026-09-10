cmake_minimum_required(VERSION 3.15)

# GoogleTest, handled like Eigen in src/CMakeLists.txt: prefer a system install,
# fetch it if there is none, and if neither is possible skip the C++ test target
# rather than failing the whole configure -- someone who only wants the library
# should not need a test framework installed.
find_package(GTest QUIET)
option(MDANCE_FETCH_GTEST "Download GoogleTest when none is installed" ON)
if(NOT GTest_FOUND AND MDANCE_FETCH_GTEST)
    message(STATUS "GoogleTest not found; fetching v1.15.2")
    include(FetchContent)
    # Keep GoogleTest out of this project's install set, and off Windows' CRT
    # mismatch warning. GoogleTest builds its own tests only with
    # gtest_build_tests=ON (default OFF), so it adds no ctest entries here.
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)
    FetchContent_MakeAvailable(googletest)
endif()

# Target names differ by source: a modern FindGTest and the fetched build both
# provide GTest::gtest_main, the fetched build also the bare gtest_main, and an
# older FindGTest only GTest::Main.
if(TARGET GTest::gtest_main)
    set(MDANCE_GTEST_LIBS GTest::gtest GTest::gtest_main)
elseif(TARGET gtest_main)
    set(MDANCE_GTEST_LIBS gtest gtest_main)
elseif(TARGET GTest::Main)
    set(MDANCE_GTEST_LIBS GTest::GTest GTest::Main)
else()
    message(STATUS
        "GoogleTest unavailable (and MDANCE_FETCH_GTEST=OFF); skipping the C++ "
        "test target. The library still builds.")
    return()
endif()

set(MDANCE_TEST_SOURCES
    test_utils.cpp
    test_main.cpp
    test_kmeans.cpp
    test_helm.cpp
)

# Renamed from `test`: a target called `test` shadows the conventional
# `make test` / ctest driver target that enable_testing() provides.
add_executable(mdance_gtests ${MDANCE_TEST_SOURCES})
target_link_libraries(mdance_gtests PRIVATE ${MDANCE_GTEST_LIBS} mdance)
# GoogleTest 1.15 requires C++14; the rest of this project is C++17.
target_compile_features(mdance_gtests PRIVATE cxx_std_17)

# The gtest fixtures locate their data as
#   current_path().parent_path().parent_path() / "tests" / "data"
# (see the comment in test_kmeans.cpp), which only resolves when the working
# directory is two levels below the source root -- true for an in-source
# build/tests/, false for any out-of-source build dir. Running them from
# tests/data itself makes that expression resolve for every build layout.
add_test(NAME mdance_tests COMMAND mdance_gtests
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data)
