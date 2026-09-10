cmake_minimum_required(VERSION 3.20)

# Auto-fix/safeguard: Check if CMakeCache was created in another path or moved
if(EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt")
    get_filename_component(CURRENT_BIN_REAL "${CMAKE_BINARY_DIR}" REALPATH)
    file(STRINGS "${CMAKE_BINARY_DIR}/CMakeCache.txt" CACHE_DIR_LINE REGEX "^CMAKE_CACHEFILE_DIR:INTERNAL=")
    if(CACHE_DIR_LINE)
        string(REPLACE "CMAKE_CACHEFILE_DIR:INTERNAL=" "" CACHED_DIR "${CACHE_DIR_LINE}")
        get_filename_component(CACHED_DIR_REAL "${CACHED_DIR}" REALPATH)
        if(NOT "${CURRENT_BIN_REAL}" STREQUAL "${CACHED_DIR_REAL}")
            message(WARNING "Detected moved or renamed build directory (was ${CACHED_DIR}, now ${CMAKE_BINARY_DIR}). Purging stale cache...")
            file(REMOVE "${CMAKE_BINARY_DIR}/CMakeCache.txt")
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/CMakeFiles")
        endif()
    endif()
endif()

project(lq VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Core Library: liblq
# ---------------------------------------------------------------------------
set(LQ_SOURCES
    src/callsign.cpp
    src/callsign_nonstd.cpp
    src/hash.cpp
    src/locator.cpp
    src/modifier.cpp
    src/rst.cpp
    src/varicode.cpp
    src/huffman.cpp
    src/message.cpp
    src/crc.cpp
    src/ldpc.cpp
    src/transport.cpp
    src/easy.cpp
    src/intent.cpp
    src/c_api.cpp
)


add_library(lq STATIC ${LQ_SOURCES})

target_include_directories(lq PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(lq PRIVATE -ffunction-sections -fdata-sections)
    add_link_options(-Wl,--gc-sections)
endif()

option(ENABLE_COVERAGE "Enable gcov code coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(lq PUBLIC -O0 -g --coverage)
    target_link_options(lq PUBLIC --coverage)
endif()

# ---------------------------------------------------------------------------
# Example & Demonstration Programs & Verification Tools
# ---------------------------------------------------------------------------
add_executable(lq_demo examples/demo/main.cpp)
target_link_libraries(lq_demo PRIVATE lq)

add_executable(lq_transceiver examples/transceiver/main.cpp)
target_link_libraries(lq_transceiver PRIVATE lq)

add_executable(lq_matrix examples/matrix/main.cpp)
target_link_libraries(lq_matrix PRIVATE lq)

add_executable(lq_tutorial examples/tutorial/main.cpp)
target_link_libraries(lq_tutorial PRIVATE lq)

add_executable(lq_c_api_demo examples/c_api_demo/main.c)
target_link_libraries(lq_c_api_demo PRIVATE lq m)

add_executable(verify_paper_examples tools/verify_paper_examples.cpp)
target_link_libraries(verify_paper_examples PRIVATE lq)
add_test(NAME verify_paper_examples COMMAND verify_paper_examples)

add_executable(export_tones tools/export_tones.cpp)
target_link_libraries(export_tones PRIVATE lq m)

add_executable(generate_test_battery tools/generate_test_battery.cpp)
target_link_libraries(generate_test_battery PRIVATE lq)

add_executable(generate_golden_vectors tools/generate_golden_vectors.cpp)
target_link_libraries(generate_golden_vectors PRIVATE lq)

add_executable(verify_golden_vectors tools/verify_golden_vectors.cpp)
target_link_libraries(verify_golden_vectors PRIVATE lq)

add_executable(benchmark_suite tools/benchmark_suite.cpp)
target_link_libraries(benchmark_suite PRIVATE lq m)

# ---------------------------------------------------------------------------
# Unit & Integration Tests (GoogleTest via FetchContent)
# ---------------------------------------------------------------------------
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(CTest)
enable_testing()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps_cache/googletest-src/CMakeLists.txt")
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/.deps_cache/googletest-src"
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-build"
        EXCLUDE_FROM_ALL
    )
else()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif()

set(TEST_SOURCES
    tests/test_callsign.cpp
    tests/test_callsign_nonstd.cpp
    tests/test_hash.cpp
    tests/test_locator.cpp
    tests/test_modifier.cpp
    tests/test_rst.cpp
    tests/test_varicode.cpp
    tests/test_huffman.cpp
    tests/test_message.cpp
    tests/test_crc.cpp
    tests/test_ldpc.cpp
    tests/test_transport.cpp
    tests/test_end_to_end.cpp
    tests/test_easy_api.cpp
    tests/test_comprehensive_matrix.cpp
    tests/test_coverage_edge_cases.cpp
    tests/test_call_reply_combinations.cpp
    tests/test_c_api.cpp
    tests/test_waterfall_dsp.cpp
    tests/test_parallel_decoder.cpp
    tests/test_battery_validation.cpp
    tests/test_golden_vectors.cpp
)



foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE lq GTest::gtest_main m)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

