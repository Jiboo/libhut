cmake_minimum_required(VERSION 3.6)
project(hut)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -std=c++1z")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/deps/")

message("Checking for required dependencies...")

find_package(Threads REQUIRED)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Vulkan_LIBRARIES})

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLM_LIBRARIES})

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if (GLSLANG_VALIDATOR EQUAL "GLSLANG_VALIDATOR-NOTFOUND")
  message(FATAL_ERROR "Can't find glslangValidator, needed for shader compilation.")
endif ()

message("Checking for optional dependencies...")

find_package(GTest)
include_directories(${GTEST_INCLUDE_DIR})

find_package(XCB COMPONENTS xcb xcb-keysyms)
if (XCB_FOUND AND xcb-keysyms_FOUND)
  message("Using XCB as backend")
  include_directories(${XCB_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${XCB_LIBRARIES})

  file(GLOB HUT_XCB_SOURCES src/xcb/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_XCB_SOURCES})
  set(HUT_PLATFORM XCB)
endif ()

if (NOT HUT_PLATFORM)
  message(FATAL_ERROR "Couldn't find a suitable backend")
endif ()

message("Generating build targets...")

file(GLOB HUT_SHADER_SOURCES spv/*.frag spv/*.vert)
STRING(REPLACE ".frag" ".frag.spv" HUT_SHADER_SPV "${HUT_SHADER_SOURCES}")
STRING(REPLACE ".vert" ".vert.spv" HUT_SHADER_SPV "${HUT_SHADER_SPV}")

FOREACH (shader_source ${HUT_SHADER_SOURCES})
  STRING(REPLACE ".frag" ".frag.spv" shader_spv "${shader_source}")
  STRING(REPLACE ".vert" ".vert.spv" shader_spv "${shader_spv}")
  add_custom_command(OUTPUT ${shader_spv}
      COMMAND ${GLSLANG_VALIDATOR} -V ${shader_source} -o ${shader_spv}
      DEPENDS ${shader_source}
      COMMENT "Generating SPIR-V for ${shader_source}")
ENDFOREACH ()

add_custom_target(shaders_spv DEPENDS ${HUT_SHADER_SPV}
    COMMENT "Checking if re-compilation of shaders is required")

add_definitions(-Ofast)
add_definitions(-DVK_USE_PLATFORM_${HUT_PLATFORM}_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/*.hpp)
file(GLOB HUT_COMMON_SOURCES src/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES})

include_directories(inc)

add_library(hut ${HUT_HEADERS} ${HUT_SOURCES})
add_dependencies(hut shaders_spv)
target_link_libraries(hut ${LIBS})

add_executable(demo tst/demo.cpp)
target_link_libraries(demo hut)
#add_custom_command(TARGET demo
#        POST_BUILD
#        COMMAND demo
#        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if (GTEST_FOUND)
  message("Enabling tests...")
  enable_testing()

  file(GLOB HUT_UNIT_TESTS tst/unit/*.cpp)
  add_executable(unittests ${HUT_UNIT_TESTS})
  add_test(NAME unittests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND unittests)
  target_link_libraries(unittests hut ${GTEST_BOTH_LIBRARIES})
endif ()
