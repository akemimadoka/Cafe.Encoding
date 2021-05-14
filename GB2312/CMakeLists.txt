add_library(Cafe.Encoding.GB2312 INTERFACE)

target_include_directories(Cafe.Encoding.GB2312 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.GB2312 INTERFACE Cafe.Encoding.Base)

AddCafeSharedFlags(Cafe.Encoding.GB2312)

install(TARGETS Cafe.Encoding.GB2312
    EXPORT Encoding.GB2312)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.GB2312
    DESTINATION cmake/Encoding.GB2312)
