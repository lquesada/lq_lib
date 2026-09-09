# =============================================================================
# The LQ Digital Mode Family — Reference Implementation (lq_lib)
#
# Author:  Luis Quesada (HB9IPH)
# Web:     https://luisquesada.com
# Portal:  https://lquesada.github.io/lq_lib/
# GitHub:  https://github.com/lquesada/lq_lib
# App:     qFT8 — Portable Amateur Radio for Android (https://qft8.com)
#
# License: MIT License (https://github.com/lquesada/lq_lib/blob/main/LICENSE)
#
# Copyright (c) 2026 Luis Quesada (HB9IPH)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# =============================================================================

cmake_minimum_required(VERSION 3.20)
project(test_ft8_lib LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Parent LQ library include directory
set(LQ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
set(LQ_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../build")

include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ft8_lib C sources
set(FT8_LIB_SOURCES
    ft8_lib/ft8/constants.c
    ft8_lib/ft8/crc.c
    ft8_lib/ft8/encode.c
    ft8_lib/ft8/decode.c
    ft8_lib/ft8/ldpc.c
    ft8_lib/ft8/message.c
    ft8_lib/ft8/text.c
    ft8_lib/common/wave.c
    ft8_lib/fft/kiss_fft.c
    ft8_lib/fft/kiss_fftr.c
)

add_library(ft8_ref STATIC ${FT8_LIB_SOURCES})
target_include_directories(ft8_ref PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ft8_lib
    ${CMAKE_CURRENT_SOURCE_DIR}/ft8_lib/ft8
    ${CMAKE_CURRENT_SOURCE_DIR}/ft8_lib/common
    ${CMAKE_CURRENT_SOURCE_DIR}/ft8_lib/fft
)
target_compile_definitions(ft8_ref PRIVATE -DLOG_LEVEL=LOG_FATAL)
target_link_libraries(ft8_ref PUBLIC m)

# Build parent lq sources
set(LQ_SOURCES
    ../../src/callsign.cpp
    ../../src/callsign_nonstd.cpp
    ../../src/hash.cpp
    ../../src/locator.cpp
    ../../src/modifier.cpp
    ../../src/rst.cpp
    ../../src/varicode.cpp
    ../../src/huffman.cpp
    ../../src/message.cpp
    ../../src/crc.cpp
    ../../src/ldpc.cpp
    ../../src/transport.cpp
    ../../src/easy.cpp
)
add_library(lq_compat STATIC ${LQ_SOURCES})
target_include_directories(lq_compat PUBLIC ${LQ_INCLUDE_DIR})

add_executable(test_ft8_compat test_ft8_compat.cpp)
target_include_directories(test_ft8_compat PRIVATE
    ${LQ_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ft8_lib
)
target_link_libraries(test_ft8_compat PRIVATE
    lq_compat
    ft8_ref
    GTest::gtest_main
    m
)

add_test(NAME test_ft8_compat COMMAND test_ft8_compat)
