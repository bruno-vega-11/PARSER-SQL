cmake_minimum_required(VERSION 3.20)
project(rtree)

set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(
        "C:/Users/USUARIO/CLionProjects/libspatialindex"
        "libspatialindex-build"
)

add_executable(rtree main.cpp
        RangeSearchVisitor.h
        KNNVisitor.h
        KNNVisitor.h
        MiPagedDiskStorageManager.h)

target_include_directories(rtree PRIVATE
        "C:/Users/USUARIO/CLionProjects/libspatialindex/include"
)

target_link_libraries(rtree PRIVATE spatialindex)