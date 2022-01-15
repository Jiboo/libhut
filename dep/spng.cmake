find_package(ZLIB)

file(GLOB SPNG_HEADERS dep/libspng/spng/*.h)
file(GLOB SPNG_SOURCES dep/libspng/spng/*.c)

set(SPNG_INCLUDES dep/libspng/spng)

set(SPNG_DEFINITIONS "")

add_library(SimplePNG STATIC ${SPNG_HEADERS} ${SPNG_SOURCES})
target_compile_definitions(SimplePNG PUBLIC ${SPNG_DEFINITIONS})
target_include_directories(SimplePNG PUBLIC ${SPNG_INCLUDES} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(SimplePNG ${ZLIB_LIBRARIES})
target_precompile_headers(SimplePNG PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dep/libspng/spng/spng.h>")
