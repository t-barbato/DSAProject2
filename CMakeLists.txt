
cmake_minimum_required(VERSION 3.16)
project(GainesvilleCrimeDatabase LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BTREE_SOURCES
        BTree/BTree.cpp
)

add_executable(CrimeDatabase
        main.cpp
        ${BTREE_SOURCES}
)

target_include_directories(CrimeDatabase PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(TreeTests
        tests.cpp
        ${BTREE_SOURCES}
)

target_include_directories(TreeTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)


option(BUILD_SFML_GUI "Build the graphical SFML application" OFF)

if(BUILD_SFML_GUI)
    find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

    add_executable(CrimeDatabaseGUI
            gui_main.cpp
            ${BTREE_SOURCES}
    )

    target_include_directories(CrimeDatabaseGUI PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(CrimeDatabaseGUI PRIVATE
            SFML::Graphics
            SFML::Window
            SFML::System
    )
endif()

enable_testing()
add_test(NAME TreeTests COMMAND TreeTests)
