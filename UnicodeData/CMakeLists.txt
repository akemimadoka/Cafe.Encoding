﻿set(CAFE_ENCODING_UNICODE_DATA_INCLUDE_RUNTIME ON CACHE BOOL "Include runtime unicode data")

# 暂时无用
set(CAFE_ENCODING_UNICODE_DATA_INCLUDE_COMPILE_TIME OFF CACHE BOOL "Include compile time unicode data")

if(CAFE_ENCODING_UNICODE_DATA_INCLUDE_RUNTIME)
    set(SOURCE_FILES
        src/Cafe/Encoding/RuntimeUnicodeData.cpp)
endif()

set(HEADERS
    src/Cafe/Encoding/RuntimeUnicodeData.h
    src/Cafe/Encoding/UnicodeData.h
    src/Cafe/Encoding/UnicodeDataTypes.h)

set(IMPL_HEADERS
    src/Cafe/Encoding/Impl/UnicodeData.h)

add_library(Cafe.Encoding.UnicodeData ${SOURCE_FILES} ${HEADERS} ${IMPL_HEADERS})

target_include_directories(Cafe.Encoding.UnicodeData PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.UnicodeData PUBLIC
    Cafe.Encoding.UTF-8)

install(TARGETS Cafe.Encoding.UnicodeData
    EXPORT Encoding.UnicodeData)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.UnicodeData
    DESTINATION cmake/Encoding.UnicodeData)
