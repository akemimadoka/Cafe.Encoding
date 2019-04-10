# set(SOURCE_FILES src/Strings.cpp)

# set(HEADERS
#     include/Encoding/CodePage.h
#     include/Encoding/CodePointIterator.h
#     include/Encoding/Encode.h
#     include/Encoding/Exceptions.h
#     include/Encoding/Strings.h)

add_library(Cafe.Encoding.Base INTERFACE)

target_include_directories(Cafe.Encoding.Base INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.Base INTERFACE
    Cafe.Core CONAN_PKG::gsl_microsoft)

target_compile_features(Cafe.Encoding.Base INTERFACE cxx_std_20)

install(TARGETS Cafe.Encoding.Base
    EXPORT Encoding.Base)

install(DIRECTORY include/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h")

install(EXPORT Encoding.Base
    NAMESPACE Cafe.
    DESTINATION cmake/Encoding.Base)
