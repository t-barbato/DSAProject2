cmake_minimum_required(VERSION 4.3)
project(DSAProject2)

set(CMAKE_CXX_STANDARD 20)

include_directories(Trees)

add_executable(DSAProject2
        Trees/BTree.cpp
        Trees/BTree.h
        Trees/BPlusTree.h
        main.cpp
        Trees/Node.h)
