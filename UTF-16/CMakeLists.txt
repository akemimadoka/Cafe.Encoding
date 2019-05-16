add_library(Cafe.Encoding.UTF-16 INTERFACE)

target_include_directories(Cafe.Encoding.UTF-16 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.UTF-16 INTERFACE Cafe.Encoding.Base)

install(TARGETS Cafe.Encoding.UTF-16
    EXPORT Encoding.UTF-16)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.UTF-16
    DESTINATION cmake/Encoding.UTF-16)
