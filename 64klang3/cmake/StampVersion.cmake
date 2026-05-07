# StampVersion.cmake
# Called as a PRE_BUILD script. Runs git to get the current short commit hash
# and writes it into version.h via configure_file.
#
# Expected variables (passed via -D from add_custom_command):
#   SRC_DIR          — path to the 64klang3 source directory (contains version.h.in)
#   OUT_FILE         — full path to write the generated version.h
#   PROJECT_VERSION  — version string from the CMake project() call (e.g. "3.0.0")

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${SRC_DIR}"
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT
)

if(NOT GIT_RESULT EQUAL 0 OR NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

configure_file("${SRC_DIR}/src/vst3/version.h.in" "${OUT_FILE}" @ONLY)
