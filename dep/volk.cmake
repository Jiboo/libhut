file(GLOB VOLK_HEADERS dep/volk/*.h)
file(GLOB VOLK_SOURCES dep/volk/*.c)

set(VOLK_INCLUDES dep/volk)

set(VOLK_DEFINITIONS "VK_NO_PROTOTYPES")

add_library(volk STATIC ${VOLK_HEADERS} ${VOLK_SOURCES})
target_compile_definitions(volk PUBLIC ${VOLK_DEFINITIONS})
target_include_directories(volk PUBLIC ${VOLK_INCLUDES})
target_precompile_headers(volk PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dep/volk/volk.h>")
set_property(TARGET volk PROPERTY EXPORT_COMPILE_COMMANDS OFF)
