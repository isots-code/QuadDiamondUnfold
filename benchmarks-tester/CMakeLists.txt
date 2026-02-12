# ------------------------------------------------------------
# Build “benchmarks-tester” as an executable.
# Links against the Common library produced above.
# ------------------------------------------------------------
add_executable(BenchmarksTester)

list(APPEND CMAKE_PREFIX_PATH "/usr/local")

find_package(benchmark REQUIRED)

# ---- Source files ---------------------------------------------------
file(GLOB_RECURSE BENCHMARK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)
target_sources(BenchmarksTester PRIVATE ${BENCHMARK_SOURCES})

# ---- Include directories -----------------------------------------------
target_include_directories(
    BenchmarksTester PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---- Link the static library from "common" ----------------------------
target_link_libraries(BenchmarksTester PRIVATE Common benchmark::benchmark)

target_compile_options(BenchmarksTester PRIVATE -fsanitize=address)
target_link_options(BenchmarksTester PRIVATE -fsanitize=address)