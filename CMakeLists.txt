cmake_minimum_required(VERSION 3.14)
project(hut)

###########################################################
# Configuration
###########################################################

#add_definitions(-DHUT_PREFER_NONDESCRETE_DEVICES)
#set(HUT_PREFER_XCB 1)

###########################################################
# Debug
###########################################################

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#add_definitions(-DHUT_ENABLE_VALIDATION)
#add_definitions(-DHUT_ENABLE_VALIDATION_DEBUG)
#add_definitions(-DHUT_DEBUG_STAGING)

###########################################################
message("Checking for required dependencies...")
###########################################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_COMPILE_WARNING_MODE HIGH)
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/deps/")

find_package(Threads REQUIRED)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Vulkan_LIBRARIES})

find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})
add_definitions("-DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_LEFT_HANDED")

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${PNG_LIBRARIES})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
set(LIBS ${LIBS} ${FREETYPE_LIBRARIES})

find_package(HarfBuzz REQUIRED)
include_directories(${HARFBUZZ_INCLUDE_DIRS})
set(LIBS ${LIBS} ${HARFBUZZ_LIBRARIES})

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "Can't find glslangValidator, needed for shader compilation.")
endif()

###########################################################
message("Checking for optional dependencies...")
###########################################################

find_package(GTest)
if(GTEST_FOUND)
  include_directories(${GTEST_INCLUDE_DIR})
endif()

find_package(Wayland)
find_package(XCB COMPONENTS xcb xcb-keysyms)

if(WAYLAND_CLIENT_FOUND AND NOT HUT_PREFER_XCB)
  message("Using Wayland as backend, checking for other dependencies..")

  find_package(XkbCommon REQUIRED)
  include_directories(${XKBCOMMON_INCLUDE_DIRS})
  add_definitions(${XKBCOMMON_DEFINITIONS})
  set(LIBS ${LIBS} ${XKBCOMMON_LIBRARIES})

  find_package(PkgConfig REQUIRED)
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
  pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
  set(XDG_SHELL_PROTOCOL ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml)
  set(WAYLAND_GEN_DIR ${CMAKE_BINARY_DIR}/wayland)
  file(MAKE_DIRECTORY ${WAYLAND_GEN_DIR})
  set(XDG_SHELL_CLIENT_INCLUDE ${WAYLAND_GEN_DIR}/xdg-shell-client-protocol.h)
  set(XDG_SHELL_SOURCE ${WAYLAND_GEN_DIR}/xdg-shell-protocol.c)
  add_custom_command(
    OUTPUT ${XDG_SHELL_CLIENT_INCLUDE}
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_PROTOCOL} ${XDG_SHELL_CLIENT_INCLUDE}
    DEPENDS ${XDG_SHELL_PROTOCOL})
  add_custom_command(
    OUTPUT ${XDG_SHELL_SOURCE}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_PROTOCOL} ${XDG_SHELL_SOURCE}
    DEPENDS ${XDG_SHELL_PROTOCOL})
  include_directories(${WAYLAND_GEN_DIR})

  include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
  set(LIBS ${LIBS} ${WAYLAND_CLIENT_LIBRARIES})
  file(GLOB HUT_WAYLAND_SOURCES src/wayland/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_WAYLAND_SOURCES} ${XDG_SHELL_CLIENT_INCLUDE} ${XDG_SHELL_SOURCE})
  set(HUT_PLATFORM WAYLAND)
elseif(XCB_FOUND AND xcb-keysyms_FOUND)
  message("Using XCB as backend")
  include_directories(${XCB_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${XCB_LIBRARIES})
  file(GLOB HUT_XCB_SOURCES src/xcb/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_XCB_SOURCES})
  set(HUT_PLATFORM XCB)
elseif(WIN32)
  message("Using WIN32 as backend")
  add_definitions(-D_CRT_SECURE_NO_WARNINGS=1 -DNOMINMAX=1) # glm related
  add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING=1) # string to_utf8(char32_t)
  add_definitions(-DWIN32_LEAN_AND_MEAN=1)
  file(GLOB HUT_WIN32_SOURCES src/win32/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_WIN32_SOURCES})
  set(HUT_PLATFORM WIN32)
endif()

if(NOT HUT_PLATFORM)
  message(FATAL_ERROR "Couldn't find a suitable backend")
endif()

###########################################################
message("Configuring shader generation...")
###########################################################

add_executable(gen_res src/tools/gen_res/main.cpp)
target_link_libraries(gen_res)

set(HUT_GEN_DIR "${CMAKE_BINARY_DIR}/gen")
include_directories(${HUT_GEN_DIR})

file(GLOB HUT_SHADER_SOURCES res/spv/*.frag res/spv/*.vert)
set(HUT_SHADER_SPV "")
file(MAKE_DIRECTORY ${HUT_GEN_DIR}/res/spv)

set(GLSLANG_VALIDATOR_FLAGS "")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(GLSLANG_VALIDATOR_FLAGS "${GLSLANG_VALIDATOR_FLAGS} -g")
endif()

foreach(shader_source ${HUT_SHADER_SOURCES})
  get_filename_component(shader_target ${shader_source} NAME)
  string(REPLACE ".frag" ".frag.spv" shader_target "${shader_target}")
  string(REPLACE ".vert" ".vert.spv" shader_target "${shader_target}")
  set(shader_target ${HUT_GEN_DIR}/res/spv/${shader_target})
  set(HUT_SHADER_SPV ${HUT_SHADER_SPV};${shader_target})

  add_custom_command(OUTPUT ${shader_target}
      COMMAND ${GLSLANG_VALIDATOR} ${GLSLANG_VALIDATOR_FLAGS} -V ${shader_source} -o ${shader_target}
      DEPENDS ${shader_source} ${HUT_GEN_DIR}/res/spv
      COMMENT "Generating SPIR-V for ${shader_source}")
endforeach()

add_custom_target(shaders_spv DEPENDS ${HUT_SHADER_SPV}
    COMMENT "Checked if shaders need recompilation")

add_custom_command(OUTPUT ${HUT_GEN_DIR}/hutgen_spv.cpp
    COMMAND $<TARGET_FILE:gen_res> hutgen_spv ${HUT_GEN_DIR} ${HUT_SHADER_SPV}
    DEPENDS gen_res ${HUT_SHADER_SPV}
    COMMENT "Generating hutgen_spv.cpp")

add_custom_target(hutgen_spv_cpp DEPENDS ${HUT_GEN_DIR}/hutgen_spv.cpp
    COMMENT "Checked if hutgen_spv.cpp need regeneration")
add_dependencies(hutgen_spv_cpp gen_res)

###########################################################
message("Configuring library build targets...")
###########################################################

add_definitions(-DVK_USE_PLATFORM_${HUT_PLATFORM}_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/hut/*.hpp inc/hut/drawables/*.hpp ${HUT_GEN_DIR}/*.hpp)
file(GLOB HUT_COMMON_SOURCES src/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES} ${HUT_GEN_DIR}/hutgen_spv.cpp)

include_directories(inc)
add_library(hut ${HUT_HEADERS} ${HUT_SOURCES})
target_link_libraries(hut ${LIBS})
add_dependencies(hut hutgen_spv_cpp)

###########################################################
message("Configuring demo build targets...")
###########################################################

file(GLOB HUT_DEMO_PNG tst/png/*.png)
add_custom_command(OUTPUT ${HUT_GEN_DIR}/demo_png.cpp
    COMMAND $<TARGET_FILE:gen_res> demo_png ${HUT_GEN_DIR} ${HUT_DEMO_PNG}
    DEPENDS gen_res ${HUT_DEMO_PNG}
    COMMENT "Generating demo_png.cpp")

file(GLOB_RECURSE HUT_DEMO_TTF tst/ttf/*.ttf)
add_custom_command(OUTPUT ${HUT_GEN_DIR}/demo_ttf.cpp
    COMMAND $<TARGET_FILE:gen_res> demo_ttf ${HUT_GEN_DIR} ${HUT_DEMO_TTF}
    DEPENDS gen_res ${HUT_DEMO_TTF}
    COMMENT "Generating demo_ttf.cpp")

add_custom_target(gen_demo_cpp DEPENDS
    ${HUT_GEN_DIR}/demo_png.cpp
    ${HUT_GEN_DIR}/demo_ttf.cpp
    COMMENT "Checked if resources need regeneration")

add_executable(demo tst/demo.cpp ${HUT_GEN_DIR}/demo_png.cpp ${HUT_GEN_DIR}/demo_ttf.cpp)
target_link_libraries(demo hut)
add_dependencies(demo gen_demo_cpp)

if(GTEST_FOUND)
  ###########################################################
  message("Enabling tests...")
  ###########################################################

  enable_testing()

  file(GLOB HUT_UNIT_TESTS tst/unit/*.cpp)
  add_executable(unittests ${HUT_UNIT_TESTS})
  add_test(NAME unittests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND unittests)
  target_link_libraries(unittests hut ${GTEST_BOTH_LIBRARIES})
endif()
