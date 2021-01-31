set(IMGUI_DEFINITIONS "")
file(GLOB IMGUI_HEADERS dep/imgui/*.h)
file(GLOB IMGUI_SOURCES dep/imgui/*.cpp)
set(IMGUI_INCLUDES dep/imgui)

add_library(imgui STATIC ${IMGUI_HEADERS} ${IMGUI_SOURCES})
target_compile_definitions(imgui PUBLIC ${IMGUI_DEFINITIONS})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDES})
target_precompile_headers(imgui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/dep/imgui/imgui.h>")
