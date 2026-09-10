cmake_minimum_required(VERSION 3.15)

enable_language(C CXX)

# Eigen discovery, in three steps so a fresh checkout builds with no manual setup.
#
# 1. Prefer a system Eigen 3.3+. 2. Eigen's own config declares same-major-version
# compatibility, so a bare "3.3" request is REJECTED by Eigen 5.x even though this
# code builds and passes its tests against it -- so retry without the floor.
# 3. If there is no Eigen at all, fetch it. Eigen is header-only, so we populate
# the sources and declare the imported target ourselves rather than running
# Eigen's own CMake, which would add its tests, docs and install rules to this
# build. Set MDANCE_FETCH_EIGEN=OFF to require a system Eigen instead.
find_package(Eigen3 3.3 QUIET)
if(NOT Eigen3_FOUND)
    find_package(Eigen3 QUIET)
endif()
option(MDANCE_FETCH_EIGEN "Download Eigen when none is installed" ON)
if(NOT Eigen3_FOUND)
    if(NOT MDANCE_FETCH_EIGEN)
        message(FATAL_ERROR
            "Eigen was not found and MDANCE_FETCH_EIGEN=OFF. Install Eigen "
            "(brew install eigen / apt install libeigen3-dev) or set "
            "MDANCE_FETCH_EIGEN=ON to download it.")
    endif()
    message(STATUS "Eigen not found; fetching Eigen 3.4.0")
    include(FetchContent)
    FetchContent_Declare(eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        # Eigen is header-only, and running its CMake would graft its own test
        # suite onto this build -- measured at 72 extra targets and 957 ctest
        # entries instead of 2. Pointing SOURCE_SUBDIR at a header directory
        # with no CMakeLists.txt makes FetchContent populate the sources and
        # skip add_subdirectory entirely.
        SOURCE_SUBDIR Eigen)
    FetchContent_MakeAvailable(eigen)
    if(NOT TARGET Eigen3::Eigen)
        add_library(Eigen3::Eigen INTERFACE IMPORTED)
        target_include_directories(Eigen3::Eigen INTERFACE "${eigen_SOURCE_DIR}")
    endif()
endif()

# Create library for tools
add_library(mdance_tools STATIC
    tools/bts.cpp
    tools/cluster.cpp
    tools/hc_utils.cpp
    tools/esim.cpp
    tools/scores.cpp
)
target_include_directories(mdance_tools PUBLIC tools)
target_link_libraries(mdance_tools PUBLIC Eigen3::Eigen)

# create library for kmeans
add_library(mdance_kmeans STATIC 
    cluster/KMeansRex/KMeans.cpp
    cluster/KMeansRex/mersenneTwister2002.c
)
set_source_files_properties(cluster/KMeansRex/mersenneTwister2002.c PROPERTIES 
    LANGUAGE C
)
target_include_directories(mdance_kmeans PUBLIC cluster/KMeansRex)
target_link_libraries(mdance_kmeans PUBLIC mdance_tools)


# create library for helm
add_library(mdance_helm STATIC 
    cluster/helm.cpp
)
target_include_directories(mdance_helm PUBLIC 
    cluster
)
target_link_libraries(mdance_helm PUBLIC mdance_tools)

# create main mdance library
add_library(mdance INTERFACE)
target_link_libraries(mdance
    INTERFACE
        mdance_tools
        mdance_kmeans
        mdance_helm
)
