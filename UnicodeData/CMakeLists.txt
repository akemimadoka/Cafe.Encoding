set(CAFE_ENCODING_UNICODE_DATA_INCLUDE_RUNTIME ON CACHE BOOL "Include runtime unicode data")

# 暂时无用
set(CAFE_ENCODING_UNICODE_DATA_INCLUDE_COMPILE_TIME OFF CACHE BOOL "Include compile time unicode data")

if(CAFE_ENCODING_UNICODE_DATA_INCLUDE_RUNTIME)
    set(SOURCE_FILES
        src/Encoding/RuntimeUnicodeData.cpp)
endif()

set(HEADERS
    include/Cafe/Encoding/RuntimeUnicodeData.h
    include/Cafe/Encoding/UnicodeData.h
    include/Cafe/Encoding/UnicodeDataTypes.h)

set(IMPL_HEADERS
    include/Cafe/Encoding/Impl/UnicodeData.h)

# 本项目编译时携带调试信息会严重影响编译速度，在有更好处理方案之前先去除相关 flag
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID MATCHES "MSVC"))
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
else()
    string(REPLACE "-g" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "-g" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()

add_library(Cafe.Encoding.UnicodeData ${SOURCE_FILES} ${HEADERS} ${IMPL_HEADERS})

target_include_directories(Cafe.Encoding.UnicodeData PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.UnicodeData PUBLIC
    Cafe.Encoding.UTF-8)

install(TARGETS Cafe.Encoding.UnicodeData
    EXPORT Encoding.UnicodeData)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.UnicodeData
    DESTINATION cmake/Encoding.UnicodeData)
