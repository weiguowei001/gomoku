cmake_minimum_required(VERSION 3.16)
project(gomoku_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    set(QT_PACKAGE Qt5)
endif()

add_executable(gomoku_qt
    ai.cpp
    board.cpp
    game.cpp
    main.cpp
)

target_link_libraries(gomoku_qt PRIVATE ${QT_PACKAGE}::Widgets)
