cmake_minimum_required(VERSION 3.14)
project(hut)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_COMPILE_WARNING_MODE HIGH)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps/")

###########################################################
# Configuration
###########################################################

#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_WINDOW_DEPTH_BUFFER)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_PREFER_NONDESCRETE_DEVICES)

set(HUT_PREFER_XCB 1)
set(HUT_COMPILE_DEMOS 1)

###########################################################
# Debug
###########################################################

#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT WIN32)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION_DEBUG)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_DEBUG_STAGING)
endif()

###########################################################
message("Checking for required dependencies...")
###########################################################

find_package(Threads REQUIRED)
set(HUT_LIBS ${HUT_LIBS} ${CMAKE_THREAD_LIBS_INIT})

find_package(Vulkan REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${Vulkan_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${Vulkan_LIBRARIES})

find_package(GLM REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${GLM_INCLUDE_DIRS})
set(HUT_DEFINITIONS ${HUT_DEFINITIONS} "-DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DGLM_FORCE_LEFT_HANDED")

find_package(PNG REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${PNG_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${PNG_LIBRARIES})

find_package(Freetype REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${FREETYPE_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${FREETYPE_LIBRARIES})

find_package(HarfBuzz REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${HARFBUZZ_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${HARFBUZZ_LIBRARIES})

find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "Can't find glslangValidator, needed for shader compilation.")
endif()
set(GLSLANG_VALIDATOR_FLAGS "")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(GLSLANG_VALIDATOR_FLAGS "${GLSLANG_VALIDATOR_FLAGS} -g")
endif()

###########################################################
message("Checking for optional dependencies...")
###########################################################

find_package(GTest)
if(GTEST_FOUND)
  set(HUT_INCLUDES_TEST ${HUT_INCLUDES} ${GTEST_INCLUDE_DIR})
endif()

find_package(Wayland COMPONENTS wayland-client wayland-cursor)
find_package(XCB COMPONENTS xcb xcb-keysyms xcb-util xcb-cursor)

if(Wayland_FOUND AND NOT HUT_PREFER_XCB)
  message("Using Wayland as backend, checking for other dependencies..")

  find_package(XkbCommon REQUIRED)
  set(HUT_INCLUDES ${HUT_INCLUDES} ${XKBCOMMON_INCLUDE_DIRS})
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} ${XKBCOMMON_DEFINITIONS})
  set(HUT_LIBS ${HUT_LIBS} ${XKBCOMMON_LIBRARIES})

  find_package(PkgConfig REQUIRED)
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
  pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
  if (NOT WAYLAND_PROTOCOLS_DIR)
    message(FATAL_ERROR "Couldn't locate wayland-protocols directory, might need to install")
  else()
    message("Found wayland-protocols: ${WAYLAND_PROTOCOLS_DIR}")
  endif()
  if (NOT WAYLAND_SCANNER)
    message(FATAL_ERROR "Couldn't locate wayland-scanner, might need to install")
  else()
    message("Found wayland-scanner: ${WAYLAND_SCANNER}")
  endif()
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
  set(HUT_INCLUDES ${HUT_INCLUDES} ${WAYLAND_GEN_DIR})

  set(HUT_INCLUDES ${HUT_INCLUDES} ${WAYLAND_INCLUDE_DIRS})
  set(HUT_LIBS ${HUT_LIBS} ${WAYLAND_LIBRARIES})
  file(GLOB HUT_WAYLAND_SOURCES src/wayland/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_WAYLAND_SOURCES} ${XDG_SHELL_CLIENT_INCLUDE} ${XDG_SHELL_SOURCE})
  set(HUT_PLATFORM WAYLAND)
elseif(XCB_FOUND)
  message("Using XCB as backend")
  set(HUT_INCLUDES ${HUT_INCLUDES} ${XCB_INCLUDE_DIRS})
  set(HUT_LIBS ${HUT_LIBS} ${XCB_LIBRARIES})
  file(GLOB HUT_XCB_SOURCES src/xcb/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_XCB_SOURCES})
  set(HUT_PLATFORM XCB)
elseif(WIN32)
  message("Using WIN32 as backend")
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -D_CRT_SECURE_NO_WARNINGS=1 -DNOMINMAX=1) # glm related
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING=1) # string to_utf8(char32_t)
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DWIN32_LEAN_AND_MEAN=1)
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

function(gen_spv_bundle)
  cmake_parse_arguments(HUT_GEN_SPV_BUNDLE "" "NAMESPACE;OUTDIR" "SHADER_SOURCES" ${ARGN})
  if (NOT HUT_GEN_SPV_BUNDLE_NAMESPACE)
    message(FATAL "No namespace specified for gen_spv_bundle")
  elseif(NOT HUT_GEN_SPV_BUNDLE_OUTDIR)
    message(FATAL "No output directory specified for gen_spv_bundle")
  elseif(NOT HUT_GEN_SPV_BUNDLE_SHADER_SOURCES)
    message(FATAL "No shader source list provided for gen_spv_bundle")
  endif()

  foreach(shader_source ${HUT_GEN_SPV_BUNDLE_SHADER_SOURCES})
    get_filename_component(shader_target ${shader_source} NAME)
    string(REPLACE ".frag" ".frag.spv" shader_target "${shader_target}")
    string(REPLACE ".vert" ".vert.spv" shader_target "${shader_target}")
    set(shader_target ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${shader_target})
    set(HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS};${shader_target})

    add_custom_command(OUTPUT ${shader_target}
            COMMAND ${GLSLANG_VALIDATOR} ${GLSLANG_VALIDATOR_FLAGS} -V ${shader_source} -o ${shader_target}
            DEPENDS ${shader_source}
            COMMENT "Generating SPIR-V for ${shader_source}")
  endforeach()

  set(bundle_target_name "bundle_${HUT_GEN_SPV_BUNDLE_NAMESPACE}")
  set(bundle_gen_name "gen_${HUT_GEN_SPV_BUNDLE_NAMESPACE}")

  add_custom_target(${bundle_target_name} DEPENDS ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          COMMENT "Checked if shaders bundle ${HUT_GEN_SPV_BUNDLE_NAMESPACE} need recompilation")

  add_custom_command(OUTPUT ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp
          COMMAND $<TARGET_FILE:gen_res> ${HUT_GEN_SPV_BUNDLE_NAMESPACE} ${HUT_GEN_SPV_BUNDLE_OUTDIR} ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          DEPENDS gen_res ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          COMMENT "Generating ${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp")

  add_custom_target(${bundle_gen_name} DEPENDS ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp
          COMMENT "Checked if ${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp need regeneration")
endfunction()

add_executable(gen_res src/tools/gen_res/main.cpp)
target_link_libraries(gen_res)
target_include_directories(gen_res PRIVATE inc)

set(HUT_GEN_DIR "${CMAKE_BINARY_DIR}/gen")
file(MAKE_DIRECTORY ${HUT_GEN_DIR})
set(HUT_INCLUDES ${HUT_INCLUDES} ${HUT_GEN_DIR})

file(GLOB HUT_SHADER_SOURCES res/spv/*.frag res/spv/*.vert)

gen_spv_bundle(NAMESPACE hut_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_SHADER_SOURCES})
set(hut_shaders_cpp ${HUT_GEN_DIR}/hut_shaders.cpp)

###########################################################
message("Configuring library build targets...")
###########################################################

set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DVK_USE_PLATFORM_${HUT_PLATFORM}_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/hut/*.hpp inc/hut/drawables/*.hpp ${HUT_GEN_DIR}/*.hpp)
file(GLOB HUT_COMMON_SOURCES src/*.cpp ${HUT_GEN_DIR}/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES})

set(HUT_INCLUDES ${HUT_INCLUDES} inc)
add_library(hut ${HUT_HEADERS} ${HUT_SOURCES} ${hut_shaders_cpp})
#message("Done configuring hut, includes: ${HUT_INCLUDES}, libs: ${HUT_LIBS}, defs: ${HUT_DEFINITIONS}")
target_link_libraries(hut ${HUT_LIBS})
target_compile_definitions(hut PUBLIC ${HUT_DEFINITIONS})
target_include_directories(hut PUBLIC ${HUT_INCLUDES})
add_dependencies(hut gen_hut_shaders)

###########################################################
message("Configuring demo build targets...")
###########################################################

if(HUT_COMPILE_DEMOS)
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

  file(GLOB HUT_DEMO_SHADERS_SOURCE tst/spv/*.vert tst/spv/*.frag)
  gen_spv_bundle(NAMESPACE demo_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_DEMO_SHADERS_SOURCE})
  set(demo_shaders_cpp ${HUT_GEN_DIR}/demo_shaders.cpp)

  add_executable(demo tst/demo.cpp ${HUT_GEN_DIR}/demo_png.cpp ${HUT_GEN_DIR}/demo_ttf.cpp ${demo_shaders_cpp})
  target_link_libraries(demo hut)
  add_dependencies(demo gen_demo_cpp gen_demo_shaders)

  add_executable(demo3d tst/demo3d.cpp ${demo_shaders_cpp})
  target_link_libraries(demo3d hut)
  add_dependencies(demo3d gen_demo_shaders)
endif()

if(GTEST_FOUND)
  ###########################################################
  message("Enabling tests...")
  ###########################################################

  enable_testing()

  file(GLOB HUT_UNIT_TESTS tst/unit/*.cpp)
  add_executable(unittests ${HUT_UNIT_TESTS})
  add_test(NAME unittests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND unittests)
  target_link_libraries(unittests hut ${GTEST_BOTH_LIBRARIES})
endif()
