cmake_minimum_required(VERSION 4.3)
project(DSAProject2)

set(CMAKE_CXX_STANDARD 20)

include_directories(BTree)

add_executable(DSAProject2
        BTree/BTree.cpp
        BTree/BTree.h
        BPlusTree.h
        BTree/Key.h
        BTree/Node.h
)
