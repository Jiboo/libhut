file(GLOB_RECURSE HUT_RENDERDOC_HEADERS *.hpp)
file(GLOB HUT_RENDERDOC_SOURCES *.cpp)
set(HUT_RENDERDOC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hut_renderdoc STATIC ${HUT_RENDERDOC_HEADERS} ${HUT_RENDERDOC_SOURCES})
target_link_libraries(hut_renderdoc dl)
target_include_directories(hut_renderdoc PUBLIC ${HUT_RENDERDOC_INCLUDES} ${RENDERDOC_INCLUDE_DIRS})
