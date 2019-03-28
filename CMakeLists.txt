set(CAFE_INCLUDE_ENCODING_LIST "UTF-8;UTF-16;UTF-32" CACHE STRING "List of include encodings")
set(CAFE_INCLUDE_UNICODE_DATA ON CACHE BOOL "Include unicode data")
set(CAFE_INCLUDE_RUNTIME_ENCODING ON CACHE BOOL "Include runtime encoding")
set(CAFE_INCLUDE_ENCODING_TEST ON CACHE BOOL "Include encoding test")

add_subdirectory(Base)

foreach(encoding ${CAFE_INCLUDE_ENCODING_LIST})
    add_subdirectory(${encoding})
endforeach()

if (CAFE_INCLUDE_UNICODE_DATA)
    if (NOT "UTF-8" IN_LIST CAFE_INCLUDE_ENCODING_LIST)
        message(SEND_ERROR "Unicode module is depend on encoding UTF-8, which is not included in CAFE_INCLUDE_ENCODING_LIST")
    endif()
    add_subdirectory(UnicodeData)
endif()

if (CAFE_INCLUDE_RUNTIME_ENCODING)
    add_subdirectory(RuntimeEncoding)
endif()

if (CAFE_INCLUDE_ENCODING_TEST)
    add_subdirectory(Test)
endif()
