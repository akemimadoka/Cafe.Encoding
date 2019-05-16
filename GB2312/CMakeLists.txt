add_library(Cafe.Encoding.GB2312 INTERFACE)

target_include_directories(Cafe.Encoding.GB2312 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.GB2312 INTERFACE Cafe.Encoding.Base)

install(TARGETS Cafe.Encoding.GB2312
    EXPORT Encoding.GB2312)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.GB2312
    DESTINATION cmake/Encoding.GB2312)
