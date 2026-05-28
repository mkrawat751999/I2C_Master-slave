# ============================================================
#  I2C Bus Simulator
#  Standalone MMIO-driven I2C master/slave emulation in C++17
# ============================================================

cmake_minimum_required(VERSION 3.14)

project(i2c_sim
    VERSION     1.0.0
    DESCRIPTION "MMIO-driven I2C master/slave bus emulation"
    LANGUAGES   CXX
)

# ---------- C++ Standard ----------
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ---------- Default Build Type ----------
# If the user didn't pass -DCMAKE_BUILD_TYPE, default to Debug.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---------- Output Directories ----------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Generate compile_commands.json (for clangd / VSCode IntelliSense)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
#  Library: i2c_core
#  Bus + Controller + Slave abstraction.
#  Kept as a static library so tests / future drivers can link
#  against it without recompiling.
# ============================================================
add_library(i2c_core STATIC
    i2c_bus.cpp
    i2c_controller.cpp
    Ii2c_slavedevice.cpp
)

target_include_directories(i2c_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------- Warnings ----------
if(MSVC)
    target_compile_options(i2c_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(i2c_core PRIVATE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wconversion -Wnon-virtual-dtor
    )
endif()

# ============================================================
#  Executable: i2c_sim
#  Wires up the bus + controller + temp sensor and runs the
#  built-in transaction tests (A–D) defined in main.cpp.
# ============================================================
add_executable(i2c_sim main.cpp)
target_link_libraries(i2c_sim PRIVATE i2c_core)

# ============================================================
#  Optional: CTest integration
#  Lets you run `ctest` from the build dir to exercise the
#  built-in test harness in main.cpp.
# ============================================================
include(CTest)
if(BUILD_TESTING)
    add_test(
        NAME    i2c_transaction_tests
        COMMAND i2c_sim
    )
    set_tests_properties(i2c_transaction_tests PROPERTIES
        PASS_REGULAR_EXPRESSION "All tests passed"
    )
endif()

# ============================================================
#  Build Summary
# ============================================================
message(STATUS "")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "  ----------------------------------------")
message(STATUS "  Build type   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler     : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Output dir   : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "")
