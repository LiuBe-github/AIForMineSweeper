cmake_minimum_required(VERSION 3.16)
project(MineSweeper VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets Network)

add_executable(MineSweeper
    src/main.cpp
    src/MineField.cpp
    src/BoardWidget.cpp
    src/MainWindow.cpp
    src/CustomGameDialog.cpp
)

target_include_directories(MineSweeper PRIVATE src)
target_link_libraries(MineSweeper PRIVATE Qt5::Widgets Qt5::Network)

if(MSVC)
    target_compile_options(MineSweeper PRIVATE /utf-8)
endif()
