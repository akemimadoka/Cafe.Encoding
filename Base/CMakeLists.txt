add_library(Cafe.Encoding.Base INTERFACE)

target_include_directories(Cafe.Encoding.Base INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(Cafe.Encoding.Base INTERFACE
    Cafe.Core CONAN_PKG::gsl_microsoft)

target_compile_features(Cafe.Encoding.Base INTERFACE cxx_std_20)
