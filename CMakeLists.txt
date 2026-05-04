cmake_minimum_required(VERSION 3.31)
project(Proyecto)

set(CMAKE_CXX_STANDARD 20)

add_executable(Proyecto main.cpp
                ast.cpp
                parser.cpp
                scanner.cpp
                token.cpp
                visitor.cpp
                BPTree.cpp
                pruebaBPTree.cpp
                SequentialFile.cpp
        ExtendibleHashing.cpp
        ExtendibleHashing.h)
