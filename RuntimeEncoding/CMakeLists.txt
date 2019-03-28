add_library(Cafe.Encoding.RuntimeEncoding INTERFACE)

set(EncodingList "")
foreach(encoding ${CAFE_INCLUDE_ENCODING_LIST})
    string(APPEND EncodingList "#include <Encoding/CodePage/${encoding}.h>\n")
    target_link_libraries(Cafe.Encoding.RuntimeEncoding INTERFACE
        "Cafe.Encoding.${encoding}")
endforeach()
configure_file(cmake/IncludedEncoding.h.in IncludedEncoding.h)

target_include_directories(Cafe.Encoding.RuntimeEncoding INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
