add_library(Cafe.Encoding.UTF-32 INTERFACE)

target_include_directories(Cafe.Encoding.UTF-32 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.UTF-32 INTERFACE Cafe.Encoding.Base)

install(TARGETS Cafe.Encoding.UTF-32
    EXPORT Encoding.UTF-32)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.UTF-32
    DESTINATION cmake/Encoding.UTF-32)
