add_library(Cafe.Encoding.UTF-8 INTERFACE)

target_include_directories(Cafe.Encoding.UTF-8 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.UTF-8 INTERFACE Cafe.Encoding.Base)

AddCafeSharedFlags(Cafe.Encoding.UTF-8)

install(TARGETS Cafe.Encoding.UTF-8
    EXPORT Encoding.UTF-8)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.UTF-8
    DESTINATION cmake/Encoding.UTF-8)
