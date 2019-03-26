set(SOURCE_FILES Main.cpp)

foreach(encoding ${CAFE_INCLUDE_ENCODING_LIST})
    list(APPEND SOURCE_FILES "${encoding}.Test.cpp")
endforeach()

if (CAFE_INCLUDE_UNICODE_DATA)
    list(APPEND SOURCE_FILES UnicodeData.Test.cpp)
endif()

add_executable(Cafe.Encoding.Test ${SOURCE_FILES})

target_link_libraries(Cafe.Encoding.Test PRIVATE
    CONAN_PKG::Catch2)

foreach(encoding ${CAFE_INCLUDE_ENCODING_LIST})
    target_link_libraries(Cafe.Encoding.Test PRIVATE
        "Cafe.Encoding.${encoding}")
endforeach()

if (CAFE_INCLUDE_UNICODE_DATA)
    target_link_libraries(Cafe.Encoding.Test PRIVATE
        Cafe.Encoding.UnicodeData)
endif()
