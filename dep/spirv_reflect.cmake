# https://github.com/KhronosGroup/SPIRV-Reflect/issues/124

file(GLOB SPIRV_REFLECT_HEADERS dep/SPIRV-Reflect/*.h)
file(GLOB SPIRV_REFLECT_SOURCES dep/SPIRV-Reflect/*.c)

set(SPIRV_REFLECT_INCLUDES dep/SPIRV-Reflect)

set(SPIRV_REFLECT_DEFINITIONS "")

add_library(spirv_reflect STATIC ${SPIRV_REFLECT_HEADERS} ${SPIRV_REFLECT_SOURCES})
target_compile_definitions(spirv_reflect PUBLIC ${SPIRV_REFLECT_DEFINITIONS})
target_include_directories(spirv_reflect PUBLIC ${SPIRV_REFLECT_INCLUDES})
target_precompile_headers(spirv_reflect PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/dep/SPIRV-Reflect/spirv_reflect.h>")
