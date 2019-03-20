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

add_library(Cafe.Encoding.UnicodeData ${SOURCE_FILES} ${HEADERS})

# get_property(CompileOptions
#     TARGET Cafe.Encoding.UnicodeData
#     PROPERTY COMPILE_FLAGS)

# message(STATUS "CompileOptions: ${CompileOptions}")

# # 移除调试信息
# list(REMOVE_ITEM CompileOptions "-g")

# set_property(TARGET Cafe.Encoding.UnicodeData
#     PROPERTY COMPILE_FLAGS ${CompileOptions})

target_include_directories(Cafe.Encoding.UnicodeData PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(Cafe.Encoding.UnicodeData PUBLIC cxx_std_17)

target_link_libraries(Cafe.Encoding.UnicodeData PUBLIC
    Cafe.Core Cafe.Encoding.UTF-8)
