add_library(Cafe.Encoding.RuntimeEncoding INTERFACE)

set(EncodingList "")
foreach(encoding ${CAFE_ENCODING_INCLUDE_ENCODING_LIST})
    string(APPEND EncodingList "#include <Cafe/Encoding/CodePage/${encoding}.h>\n")
    target_link_libraries(Cafe.Encoding.RuntimeEncoding INTERFACE
        "Cafe.Encoding.${encoding}")
endforeach()
configure_file(cmake/IncludedEncoding.h.in Cafe/Encoding/Config/IncludedEncoding.h)

target_include_directories(Cafe.Encoding.RuntimeEncoding INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

install(TARGETS Cafe.Encoding.RuntimeEncoding
    EXPORT Encoding.RuntimeEncoding)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Encoding/Config/IncludedEncoding.h
    DESTINATION include/Cafe/Encoding/Config)

install(EXPORT Encoding.RuntimeEncoding
    NAMESPACE Cafe.
    DESTINATION cmake/Encoding.RuntimeEncoding)
