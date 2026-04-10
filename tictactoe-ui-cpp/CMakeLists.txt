cmake_minimum_required(VERSION 3.14)
project(TicTacToeUiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

logos_module(
    NAME tictactoe_ui_cpp
    SOURCES
        src/tictactoe_ui_interface.h
        src/tictactoe_ui_plugin.h
        src/tictactoe_ui_plugin.cpp
        src/tictactoe_backend.h
        src/tictactoe_backend.cpp
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(tictactoe_ui_cpp_module_plugin PRIVATE Qt6::Widgets)

