cmake_minimum_required(VERSION 3.14)
project(TicTacToeUiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Compile protobuf messages
find_package(Protobuf REQUIRED)
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_SRC ${PROTO_GEN_DIR}/tictactoe.pb.cc)
set(PROTO_HDR ${PROTO_GEN_DIR}/tictactoe.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR}
    COMMAND protobuf::protoc
        --cpp_out=${PROTO_GEN_DIR}
        -I${PROTO_DIR}
        ${PROTO_DIR}/tictactoe.proto
    DEPENDS ${PROTO_DIR}/tictactoe.proto
    COMMENT "Generating protobuf sources for tictactoe"
)

logos_module(
    NAME tictactoe_ui_cpp
    SOURCES
        src/tictactoe_ui_interface.h
        src/tictactoe_ui_plugin.h
        src/tictactoe_ui_plugin.cpp
        src/tictactoe_backend.h
        src/tictactoe_backend.cpp
        ${PROTO_SRC}
        ${PROTO_HDR}
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
        ${PROTO_GEN_DIR}
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(tictactoe_ui_cpp_module_plugin PRIVATE Qt6::Widgets protobuf::libprotobuf)

