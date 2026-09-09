cmake_minimum_required(VERSION 3.22.1)
project(lq_native)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LQ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LQ_ROOT}/include
)

set(LQ_SOURCES
    ${LQ_ROOT}/src/callsign.cpp
    ${LQ_ROOT}/src/callsign_nonstd.cpp
    ${LQ_ROOT}/src/crc.cpp
    ${LQ_ROOT}/src/easy.cpp
    ${LQ_ROOT}/src/hash.cpp
    ${LQ_ROOT}/src/huffman.cpp
    ${LQ_ROOT}/src/ldpc.cpp
    ${LQ_ROOT}/src/locator.cpp
    ${LQ_ROOT}/src/message.cpp
    ${LQ_ROOT}/src/modifier.cpp
    ${LQ_ROOT}/src/rst.cpp
    ${LQ_ROOT}/src/transport.cpp
    ${LQ_ROOT}/src/varicode.cpp
)

add_library(lq_native SHARED
    lq_jni.cpp
    ${LQ_SOURCES}
)

find_library(log-lib log)
target_link_libraries(lq_native ${log-lib})

# Support 16 KB page sizes for Android 15+
target_link_options(lq_native PRIVATE "-Wl,-z,max-page-size=16384")
