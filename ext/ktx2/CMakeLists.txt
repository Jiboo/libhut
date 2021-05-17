file(GLOB_RECURSE HUT_KTX2_HEADERS *.hpp)
file(GLOB_RECURSE HUT_KTX2_SOURCES *.cpp)
set(HUT_KTX2_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_library(hut_ktx2 STATIC ${HUT_KTX2_HEADERS} ${HUT_KTX2_SOURCES})
target_link_libraries(hut_ktx2 hut)
target_include_directories(hut_ktx2 PUBLIC ${HUT_KTX2_INCLUDES})
