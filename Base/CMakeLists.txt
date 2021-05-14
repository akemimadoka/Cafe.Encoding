add_library(Cafe.Encoding.Base INTERFACE)

target_include_directories(Cafe.Encoding.Base INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Cafe.Encoding.Base INTERFACE
    CONAN_PKG::Cafe.Core
)

target_compile_features(Cafe.Encoding.Base INTERFACE cxx_std_20)

AddCafeSharedFlags(Cafe.Encoding.Base)

install(TARGETS Cafe.Encoding.Base
    EXPORT Encoding.Base
)

install(DIRECTORY src/
    TYPE INCLUDE
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT Encoding.Base
    DESTINATION cmake/Encoding.Base
)
