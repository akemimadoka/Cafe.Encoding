set(SOURCE_FILES
    src/RuntimeEncoding.cpp)

set(HEADERS
    include/Cafe/Encoding/RuntimeEncoding.h)

set(EncodingList "")
foreach(encoding ${CAFE_ENCODING_INCLUDE_ENCODING_LIST})
    string(APPEND EncodingList "#include <Cafe/Encoding/CodePage/${encoding}.h>\n")
endforeach()
configure_file(cmake/IncludedEncoding.h.in Cafe/Encoding/Config/IncludedEncoding.h)

add_library(Cafe.Encoding.RuntimeEncoding ${SOURCE_FILES} ${HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Encoding/Config/IncludedEncoding.h)

target_include_directories(Cafe.Encoding.RuntimeEncoding PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

foreach(encoding ${CAFE_ENCODING_INCLUDE_ENCODING_LIST})
    target_link_libraries(Cafe.Encoding.RuntimeEncoding PUBLIC
        "Cafe.Encoding.${encoding}")
endforeach()

install(TARGETS Cafe.Encoding.RuntimeEncoding
    EXPORT Encoding.RuntimeEncoding)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Cafe/Encoding/Config/IncludedEncoding.h
    DESTINATION include/Cafe/Encoding/Config)

install(EXPORT Encoding.RuntimeEncoding
    DESTINATION cmake/Encoding.RuntimeEncoding)
