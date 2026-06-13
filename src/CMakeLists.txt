cmake_minimum_required(VERSION 3.15)

enable_language(C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Eigen3 REQUIRED)

# Create library for tools
add_library(mdance_tools STATIC
    tools/bts.cpp
    tools/cluster.cpp
    tools/hc_utils.cpp
    tools/esim.cpp
    tools/scores.cpp
    tools/result_utils.cpp
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

# create divine library
add_library(mdance_divine STATIC
    cluster/divine.cpp
)
target_link_libraries(mdance_divine PUBLIC mdance_tools)

# create equal library
add_library(mdance_equal STATIC
    cluster/equal.cpp
)
target_link_libraries(mdance_equal PUBLIC mdance_tools)

# create prime library
add_library(mdance_prime STATIC
    cluster/prime.cpp
)
target_link_libraries(mdance_prime PUBLIC mdance_tools)

# create main mdance library
add_library(mdance INTERFACE)
target_link_libraries(mdance
    INTERFACE
        mdance_tools
        mdance_kmeans
        mdance_helm
        mdance_divine
        mdance_equal
        mdance_prime
)