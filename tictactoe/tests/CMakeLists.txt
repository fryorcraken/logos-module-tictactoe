# Self-contained test build for the tictactoe core module.
#
#   cd tictactoe/tests
#   cmake -B build -S .
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Intentionally decoupled from the parent plugin build so the Logos toolchain
# is not required to run the tests.

cmake_minimum_required(VERSION 3.14)
project(tictactoe_tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../proto)

enable_testing()

# ── libtictactoe C tests ───────────────────────────────────────────
add_library(tictactoe_c STATIC ${LIB_DIR}/libtictactoe.c)
target_include_directories(tictactoe_c PUBLIC ${LIB_DIR})

add_executable(test_libtictactoe test_libtictactoe.c)
target_link_libraries(test_libtictactoe PRIVATE tictactoe_c)
add_test(NAME libtictactoe COMMAND test_libtictactoe)

# ── protobuf round-trip test ──────────────────────────────────────
find_package(Protobuf REQUIRED)

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
    COMMENT "Generating protobuf sources for tests"
)

add_executable(test_proto_roundtrip test_proto_roundtrip.cpp ${PROTO_SRC} ${PROTO_HDR})
target_include_directories(test_proto_roundtrip PRIVATE ${PROTO_GEN_DIR})
target_link_libraries(test_proto_roundtrip PRIVATE protobuf::libprotobuf)
add_test(NAME proto_roundtrip COMMAND test_proto_roundtrip)
