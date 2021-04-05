set(IMGUI_DEFINITIONS "IMGUI_USE_WCHAR32")
file(GLOB IMGUI_HEADERS dep/imgui/*.h)
file(GLOB IMGUI_SOURCES dep/imgui/*.cpp)
set(IMGUI_INCLUDES dep/imgui)

add_library(imgui STATIC ${IMGUI_HEADERS} ${IMGUI_SOURCES})
target_compile_definitions(imgui PUBLIC ${IMGUI_DEFINITIONS})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDES})
target_precompile_headers(imgui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dep/imgui/imgui.h>")

if (WINDOWS)
  target_link_libraries(imgui imm32)
endif()
