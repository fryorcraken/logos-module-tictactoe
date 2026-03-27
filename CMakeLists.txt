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

# Build libtictactoe from source — no pre-built binary needs to be committed
add_library(tictactoe SHARED lib/libtictactoe.c)
target_include_directories(tictactoe PUBLIC lib)
set_target_properties(tictactoe PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    INSTALL_RPATH "$ORIGIN"
)
install(TARGETS tictactoe
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
)

# Define the module, linking against the tictactoe target above
logos_module(
    NAME tictactoe
    SOURCES
        src/tictactoe_interface.h
        src/tictactoe_plugin.h
        src/tictactoe_plugin.cpp
    LINK_TARGETS
        tictactoe
)
