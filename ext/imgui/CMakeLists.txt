set(HUT_IMGUI_IMPL_DEFINITIONS "")
file(GLOB_RECURSE HUT_IMGUI_IMPL_HEADERS *.h)
file(GLOB_RECURSE HUT_IMGUI_IMPL_SOURCES *.cpp)
set(HUT_IMGUI_IMPL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB HUT_IMGUI_SHADERS_SOURCE *.vert *.frag)
gen_spv_bundle(NAMESPACE hut_imgui_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_IMGUI_SHADERS_SOURCE})

add_library(hut_imgui_impl STATIC ${HUT_IMGUI_IMPL_HEADERS} ${HUT_IMGUI_IMPL_SOURCES} ${HUT_GEN_DIR}/hut_imgui_shaders.cpp)
add_dependencies(hut_imgui_impl gen_hut_imgui_shaders)
target_link_libraries(hut_imgui_impl imgui hut)
target_compile_definitions(hut_imgui_impl PUBLIC ${HUT_IMGUI_IMPL_DEFINITIONS})
target_include_directories(hut_imgui_impl PUBLIC ${HUT_IMGUI_IMPL_INCLUDES})
