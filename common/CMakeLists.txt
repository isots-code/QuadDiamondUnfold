# ------------------------------------------------------------
# CMake configuration for the "common" native project.
# Produces a static library named Common.
# ------------------------------------------------------------
add_library(Common STATIC)

# If your headers reside in an `include/` sub‑folder, expose them:
target_include_directories(
    Common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/version2
)

# Add common compiler flags
target_compile_options(Common PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Ofast
    -march=native
)

# ---- Source files ------------------------------------------------
# Option 1: Add known source files explicitly (recommended for full control)
# target_sources(Common PRIVATE
#     foo.cpp
#     bar.cpp
# )

# Option 2: Gather all *.cpp / *.cxx recursively (quick & dirty)
file(GLOB_RECURSE COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)

target_sources(Common PRIVATE ${COMMON_SOURCES})

# ---- Link dependencies --------------------------------------------
# If “Common” has any third‑party deps, add them here.
# target_link_libraries(Common PUBLIC ...)
