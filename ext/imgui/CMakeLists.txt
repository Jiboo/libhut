set(HUT_IMGUI_IMPL_DEFINITIONS "")
file(GLOB_RECURSE HUT_IMGUI_IMPL_HEADERS *.hpp)
file(GLOB_RECURSE HUT_IMGUI_IMPL_SOURCES *.cpp)
set(HUT_IMGUI_IMPL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
set(HUT_IMGUI_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

file(GLOB HUT_IMGUI_SHADERS_SOURCE *.vert *.frag)
hut_gen_spv_bundle(NAMESPACE hut_imgui_shaders OUTDIR ${HUT_IMGUI_GEN_DIR} SHADER_SOURCES ${HUT_IMGUI_SHADERS_SOURCE})

add_library(hut_imgui_impl STATIC ${HUT_IMGUI_IMPL_HEADERS} ${HUT_IMGUI_IMPL_SOURCES} ${HUT_BUNDLE_hut_imgui_shaders_FILES})
target_link_libraries(hut_imgui_impl imgui hut)
target_compile_definitions(hut_imgui_impl PUBLIC ${HUT_IMGUI_IMPL_DEFINITIONS})
target_include_directories(hut_imgui_impl PUBLIC ${HUT_IMGUI_IMPL_INCLUDES} ${HUT_IMGUI_GEN_DIR})
