add_library(Cafe.Encoding.UTF-32 INTERFACE)

target_include_directories(Cafe.Encoding.UTF-32 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.UTF-32 INTERFACE Cafe.Encoding.Base)

AddCafeSharedFlags(Cafe.Encoding.UTF-32)

install(TARGETS Cafe.Encoding.UTF-32
    EXPORT Encoding.UTF-32)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.UTF-32
    DESTINATION cmake/Encoding.UTF-32)
