# ------------------------------------------------------------
# Build “runner” as an executable.
# Also links against the Common library.
# ------------------------------------------------------------
add_executable(Runner)

# ---- Source files ---------------------------------------------------
file(GLOB_RECURSE RUNNER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)
target_sources(Runner PRIVATE ${RUNNER_SOURCES})

# ---- Include directories -----------------------------------------------
target_include_directories(
    Runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # adjust if your headers live elsewhere
)

# ---- Link the static library from "common" ----------------------------
target_link_libraries(Runner PRIVATE Common)