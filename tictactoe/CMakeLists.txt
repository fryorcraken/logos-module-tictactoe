cmake_minimum_required(VERSION 3.14)
project(TicTacToeModulePlugin LANGUAGES C CXX)

# Include the Logos Module CMake helper
if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

# Build libtictactoe from source — no pre-built .so needs to be committed.
# Output to ${CMAKE_BINARY_DIR}/lib so the Nix install phase picks it up.
add_library(tictactoe SHARED lib/libtictactoe.c)
target_include_directories(tictactoe PUBLIC lib)
set_target_properties(tictactoe PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    INSTALL_RPATH "$ORIGIN"
)
install(TARGETS tictactoe
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
)

# Compile protobuf messages for multiplayer serialization
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

# Define the module, linking against the tictactoe CMake target
logos_module(
    NAME tictactoe
    SOURCES
        src/tictactoe_interface.h
        src/tictactoe_plugin.h
        src/tictactoe_plugin.cpp
        ${PROTO_SRC}
        ${PROTO_HDR}
    LINK_TARGETS
        tictactoe
    INCLUDE_DIRS
        ${PROTO_GEN_DIR}
)

target_link_libraries(tictactoe_module_plugin PRIVATE protobuf::libprotobuf)
