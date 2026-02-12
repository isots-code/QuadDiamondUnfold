# ------------------------------------------------------------
# Top‑level CMake configuration for the whole repository.
# ------------------------------------------------------------
cmake_minimum_required(VERSION 3.15)
project(Repo LANGUAGES CXX)

# Use a modern C++ standard – change if you need a different version.
set(CMAKE_C_STANDARD        17)
set(CMAKE_CXX_STANDARD      20)

# Optional: enforce stricter warnings (feel free to tweak).
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Ofast -march=native -ggdb)
    # add_compile_options(-fsanitize=address)
    # add_link_options(-fsanitize=address)
endif()

# Add the three native sub‑projects.
add_subdirectory(common        common)
add_subdirectory(benchmarks-tester benchmarks-tester)
# add_subdirectory(runner         runner)