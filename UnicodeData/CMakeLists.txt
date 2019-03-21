set(CAFE_INCLUDE_RUNTIME_UNICODE_DATA ON CACHE BOOL "Include runtime unicode data")

# 暂时无用
set(CAFE_INCLUDE_COMPILE_TIME_UNICODE_DATA OFF CACHE BOOL "Include compile time unicode data")

if(CAFE_INCLUDE_RUNTIME_UNICODE_DATA)
    set(SOURCE_FILES
        src/Encoding/RuntimeUnicodeData.cpp)
endif()

set(HEADERS
    include/Encoding/RuntimeUnicodeData.h
    include/Encoding/UnicodeData.h
    include/Encoding/UnicodeDataTypes.h)

# 本项目编译时携带调试信息会严重影响编译速度，在有更好处理方案之前先去除相关 flag
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID MATCHES "MSVC"))
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
else()
    string(REPLACE "-g" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "-g" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()

add_library(Cafe.Encoding.UnicodeData ${SOURCE_FILES} ${HEADERS})

target_include_directories(Cafe.Encoding.UnicodeData PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(Cafe.Encoding.UnicodeData PUBLIC cxx_std_17)

target_link_libraries(Cafe.Encoding.UnicodeData PUBLIC
    Cafe.Core Cafe.Encoding.UTF-8)
