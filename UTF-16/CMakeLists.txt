add_library(Cafe.Encoding.UTF-16 INTERFACE)

target_include_directories(Cafe.Encoding.UTF-16 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(Cafe.Encoding.UTF-16 INTERFACE cxx_std_20)

target_link_libraries(Cafe.Encoding.UTF-16 INTERFACE Cafe.Encoding.Base)
