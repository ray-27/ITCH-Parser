cmake_minimum_required(VERSION 3.20)
project(itch-cpp CXX)

# ── Library ───────────────────────────────────────────────────────────────
add_library(itch INTERFACE)
target_include_directories(itch INTERFACE include/)
target_compile_options(itch INTERFACE
    -O3
    -march=native
    -std=c++23
    -Wall
    -Wextra
)

# ── Benchmark — zero I/O in hot loop, measures pure parse throughput ───────
add_executable(bench tools/bench.cpp)
target_link_libraries(bench PRIVATE itch)

# ── Print — human-readable output, I/O bound by design ────────────────────
add_executable(print tools/print.cpp)
target_link_libraries(print PRIVATE itch)

# ── Dump (kept for compatibility) ─────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/dump.cpp")
    add_executable(dump tools/dump.cpp)
    target_link_libraries(dump PRIVATE itch)
endif()