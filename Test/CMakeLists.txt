set(SOURCE_FILES Main.cpp UTF-8.Test.cpp UnicodeData.Test.cpp)

add_executable(Cafe.Encoding.Test ${SOURCE_FILES})

target_link_libraries(Cafe.Encoding.Test PRIVATE
    Cafe.Encoding.UTF-8 Cafe.Encoding.UnicodeData CONAN_PKG::Catch2)
