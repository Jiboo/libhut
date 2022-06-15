file(GLOB ENTT_HEADERS dep/entt/src/*.hpp)

set(ENTT_INCLUDES dep/entt/src)

set(ENTT_DEFINITIONS "")

add_library(EnTT INTERFACE ${ENTT_HEADERS})
target_compile_definitions(EnTT INTERFACE ${ENTT_DEFINITIONS})
target_include_directories(EnTT INTERFACE ${ENTT_INCLUDES} ${ZLIB_INCLUDE_DIRS})
target_precompile_headers(EnTT INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dep/entt/src/entt/entt.hpp>")
set_property(TARGET EnTT PROPERTY EXPORT_COMPILE_COMMANDS OFF)
