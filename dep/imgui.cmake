file(GLOB IMGUI_HEADERS dep/imgui/*.h)
file(GLOB IMGUI_SOURCES dep/imgui/*.cpp)

set(IMGUI_INCLUDES dep/imgui)

set(IMGUI_DEFINITIONS
        IMGUI_USE_WCHAR32
        IMGUI_DISABLE_OBSOLETE_KEYIO
        IMGUI_DISABLE_OBSOLETE_FUNCTIONS
        IMGUI_DEBUG_PARANOID)

add_library(DearImGui STATIC ${IMGUI_HEADERS} ${IMGUI_SOURCES})
target_compile_definitions(DearImGui PUBLIC ${IMGUI_DEFINITIONS})
target_include_directories(DearImGui PUBLIC ${IMGUI_INCLUDES})
target_precompile_headers(DearImGui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/dep/imgui/imgui.h>")
