cmake_minimum_required(VERSION 3.16)
project(hut)

###########################################################
# Configuration
###########################################################
option(HUT_COMPILE_DEMOS "Compile all demos, can take some time due to shader/resources pipelines" TRUE)
option(HUT_ENABLE_IMGUI "Enable the imgui extension, which is used for some tests and playgrounds" TRUE)
option(HUT_ENABLE_KTX2 "Enable the ktx2 io extension library" TRUE)
option(HUT_ENABLE_RENDERDOC "Enable the renderdoc extension allowing to trigger captures from code" TRUE)
option(HUT_ENABLE_IMGDEC "Enable the image loading extension" TRUE)

###########################################################
# Debug
###########################################################
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
#set(HUT_ENABLE_PROFILING TRUE)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION_DEBUG)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_DEBUG_WL_DATA_LISTENERS)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_DEBUG_STAGING)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_PREFER_NONDESCRETE_DEVICES)

###########################################################
# Compile/link flags
###########################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_MODE HIGH)

###########################################################
# Checking for required dependencies...
###########################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps/")

add_subdirectory(dep/glslang)
set(GLSLANG_VALIDATOR_FLAGS "")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(GLSLANG_VALIDATOR_FLAGS "${GLSLANG_VALIDATOR_FLAGS} -g")
endif()

include(dep/spirv_reflect.cmake)

find_package(Threads REQUIRED)
set(HUT_LIBS ${HUT_LIBS} ${CMAKE_THREAD_LIBS_INIT})

find_package(Vulkan REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${Vulkan_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${Vulkan_LIBRARIES})

find_package(GLM REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${GLM_INCLUDE_DIRS})
set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DGLM_FORCE_LEFT_HANDED)

find_package(PNG REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${PNG_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${PNG_LIBRARIES})

find_package(Freetype REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${FREETYPE_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${FREETYPE_LIBRARIES})

find_package(HarfBuzz REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${HARFBUZZ_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${HARFBUZZ_LIBRARIES})

find_package(Wayland REQUIRED COMPONENTS wayland-client wayland-cursor)

find_package(XkbCommon REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${XKBCOMMON_INCLUDE_DIRS})
set(HUT_DEFINITIONS ${HUT_DEFINITIONS} ${XKBCOMMON_DEFINITIONS})
set(HUT_LIBS ${HUT_LIBS} ${XKBCOMMON_LIBRARIES})

find_package(PkgConfig REQUIRED)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
if (NOT WAYLAND_PROTOCOLS_DIR)
  message(FATAL_ERROR "Couldn't locate wayland-protocols directory, might need to install")
endif()
if (NOT WAYLAND_SCANNER)
  message(FATAL_ERROR "Couldn't locate wayland-scanner, might need to install")
endif()

set(WAYLAND_GEN_DIR ${CMAKE_BINARY_DIR}/wayland)
file(MAKE_DIRECTORY ${WAYLAND_GEN_DIR})
set(HUT_WAYLAND_GENERATED "")

set(XDG_SHELL_PROTOCOL ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml)
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
list(APPEND HUT_WAYLAND_GENERATED ${XDG_SHELL_CLIENT_INCLUDE} ${XDG_SHELL_SOURCE})

set(HUT_INCLUDES ${HUT_INCLUDES} ${WAYLAND_GEN_DIR})
set(HUT_INCLUDES ${HUT_INCLUDES} ${WAYLAND_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${WAYLAND_LIBRARIES})
file(GLOB HUT_WAYLAND_SOURCES src/wayland/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_WAYLAND_SOURCES} ${HUT_WAYLAND_GENERATED})

###########################################################
# Checking for optional dependencies...
###########################################################
if (HUT_ENABLE_PROFILING)
  find_package(fmt) #TODO Replace with std::format
  find_package(Boost)
  find_package(Backtrace) #TODO Replace with std::stacktrace

  if (fmt_FOUND AND Boost_FOUND)
    set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_PROFILING)
    set(HUT_INCLUDES ${HUT_INCLUDES} ${Boost_INCLUDE_DIRS})
    set(HUT_LIBS ${HUT_LIBS} fmt::fmt-header-only)
    set(HUT_LIBS ${HUT_LIBS} dl) # Dependency of boost-stacktrace

    if (Backtrace_FOUND)
      set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DBOOST_STACKTRACE_USE_BACKTRACE)
      set(HUT_INCLUDES ${HUT_INCLUDES} ${BACKTRACE_INCLUDE_DIRS})
      set(HUT_LIBS ${HUT_LIBS} ${BACKTRACE_LIBRARIES})
    else()
      message(WARNING "Defaulting to addr2line, which is slow to generate backtraces, please install https://github.com/ianlancetaylor/libbacktrace")
      set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DBOOST_STACKTRACE_USE_ADDR2LINE)
    endif()
  endif()
endif()

if (HUT_ENABLE_IMGUI)
  include(dep/imgui.cmake)
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_IMGUI)
endif()

if (HUT_ENABLE_IMGDEC)
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_IMGDEC)
endif()

if (HUT_ENABLE_KTX2)
  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_KTX2)
endif()

if (HUT_ENABLE_RENDERDOC)
  find_package(Renderdoc)
  if (RENDERDOC_FOUND)
    set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_RENDERDOC)
  else()
    set(HUT_ENABLE_RENDERDOC FALSE)
    message("Couldn't find renderdoc on system, disabling")
  endif()
endif()

enable_testing()
find_package(GTest REQUIRED)
if(GTEST_FOUND)
  set(HUT_INCLUDES_TEST ${HUT_INCLUDES} ${GTEST_INCLUDE_DIR})
endif()

###########################################################
# Shader generation helpers
###########################################################
function(hut_gen_spv_bundle)
  cmake_parse_arguments(HUT_GEN_SPV_BUNDLE "" "NAMESPACE;OUTDIR" "SHADER_SOURCES" ${ARGN})
  if (NOT HUT_GEN_SPV_BUNDLE_NAMESPACE)
    message(FATAL "No namespace specified for hut_gen_spv_bundle")
  elseif(NOT HUT_GEN_SPV_BUNDLE_OUTDIR)
    message(FATAL "No output directory specified for hut_gen_spv_bundle")
  elseif(NOT HUT_GEN_SPV_BUNDLE_SHADER_SOURCES)
    message(FATAL "No shader source list provided for hut_gen_spv_bundle")
  endif()

  file(MAKE_DIRECTORY ${HUT_GEN_SPV_BUNDLE_OUTDIR})

  foreach(shader_source ${HUT_GEN_SPV_BUNDLE_SHADER_SOURCES})
    get_filename_component(shader_target ${shader_source} NAME)
    string(REPLACE ".frag" ".frag.spv" shader_target "${shader_target}")
    string(REPLACE ".vert" ".vert.spv" shader_target "${shader_target}")
    set(shader_target ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${shader_target})
    set(HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS};${shader_target})

    add_custom_command(OUTPUT ${shader_target}
            COMMAND $<TARGET_FILE:glslangValidator> ${GLSLANG_VALIDATOR_FLAGS} -V ${shader_source} -o ${shader_target}
            DEPENDS ${shader_source} glslangValidator
            COMMENT "Generating SPIR-V for ${shader_source}")
  endforeach()

  add_custom_command(OUTPUT ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.hpp
          COMMAND $<TARGET_FILE:gen_res> ${HUT_GEN_SPV_BUNDLE_NAMESPACE} ${HUT_GEN_SPV_BUNDLE_OUTDIR} ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          DEPENDS gen_res ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          COMMENT "Generating ${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp/hpp")

  add_custom_command(OUTPUT ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl.hpp
          COMMAND $<TARGET_FILE:gen_spv> ${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl ${HUT_GEN_SPV_BUNDLE_OUTDIR} ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          DEPENDS gen_spv ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          COMMENT "Generating SPIR-V reflection for ${HUT_GEN_SPV_BUNDLE_NAMESPACE}")

  set(HUT_BUNDLE_${HUT_GEN_SPV_BUNDLE_NAMESPACE}_FILES
      ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
      ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp
      ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.hpp
      ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl.hpp
      PARENT_SCOPE)
endfunction()

###########################################################
# Resource generation and shader reflection
###########################################################
add_executable(gen_res src/tools/gen_res/main.cpp)
target_link_libraries(gen_res)
target_include_directories(gen_res PRIVATE inc)

add_executable(gen_spv src/tools/gen_spv/main.cpp)
target_link_libraries(gen_spv spirv_reflect)
target_include_directories(gen_spv PRIVATE inc)

###########################################################
# hut library
###########################################################
set(HUT_GEN_DIR "${CMAKE_BINARY_DIR}/gen")
file(MAKE_DIRECTORY ${HUT_GEN_DIR})
set(HUT_INCLUDES ${HUT_INCLUDES} ${HUT_GEN_DIR})

file(GLOB HUT_SHADER_SOURCES res/spv/*.frag res/spv/*.vert)

hut_gen_spv_bundle(NAMESPACE hut_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_SHADER_SOURCES})

set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DVK_USE_PLATFORM_WAYLAND_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/hut/*.hpp ${HUT_GEN_DIR}/*.hpp)
file(GLOB HUT_COMMON_SOURCES src/*.cpp ${HUT_GEN_DIR}/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES})

set(HUT_INCLUDES ${HUT_INCLUDES} inc)
add_library(hut STATIC ${HUT_HEADERS} ${HUT_SOURCES} ${HUT_BUNDLE_hut_shaders_FILES})
target_link_libraries(hut ${HUT_LIBS})
target_compile_definitions(hut PUBLIC ${HUT_DEFINITIONS})
target_include_directories(hut PUBLIC ${HUT_INCLUDES})
target_precompile_headers(hut PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/inc/hut/utils.hpp>")

#message("HUT_DEFINITIONS: ${HUT_DEFINITIONS}")
#message("HUT_INCLUDES: ${HUT_INCLUDES}")
#message("HUT_LIBS: ${HUT_LIBS}")
#message("HUT_SOURCES: ${HUT_SOURCES}")

add_subdirectory(ext)

###########################################################
# Demo targets
###########################################################
if(HUT_COMPILE_DEMOS)
  file(GLOB HUT_DEMO_PNG tst/png/*.png)
  add_custom_command(OUTPUT ${HUT_GEN_DIR}/demo_png.cpp
      COMMAND $<TARGET_FILE:gen_res> demo_png ${HUT_GEN_DIR} ${HUT_DEMO_PNG}
      DEPENDS gen_res ${HUT_DEMO_PNG}
      COMMENT "Generating demo_png.cpp")

  file(GLOB_RECURSE HUT_DEMO_WOFF2 tst/woff2/*.woff2)
  add_custom_command(OUTPUT ${HUT_GEN_DIR}/demo_woff2.cpp
      COMMAND $<TARGET_FILE:gen_res> demo_woff2 ${HUT_GEN_DIR} ${HUT_DEMO_WOFF2}
      DEPENDS gen_res ${HUT_DEMO_WOFF2}
      COMMENT "Generating demo_woff2.cpp")

  file(GLOB HUT_DEMO_SHADERS_SOURCE tst/spv/*.vert tst/spv/*.frag)
  hut_gen_spv_bundle(NAMESPACE demo_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_DEMO_SHADERS_SOURCE})

  set(HUT_DEMO_GENERATED_RES ${HUT_GEN_DIR}/demo_png.cpp ${HUT_GEN_DIR}/demo_woff2.cpp)

  add_executable(demo_3d tst/demo_3d.cpp ${HUT_BUNDLE_demo_shaders_FILES})
  target_link_libraries(demo_3d hut)

  if (HUT_ENABLE_IMGDEC)
    add_executable(demo tst/demo.cpp ${HUT_DEMO_GENERATED_RES} ${HUT_BUNDLE_demo_shaders_FILES})
    target_link_libraries(demo hut hut_imgdec)
  endif()

  if (HUT_ENABLE_IMGUI)
    add_executable(demo_imgui tst/demo_imgui.cpp ${HUT_DEMO_GENERATED_RES})
    target_link_libraries(demo_imgui hut_imgui_impl)

    add_executable(sandbox_binpacks tst/sandbox_binpacks_imgui.cpp ${HUT_DEMO_GENERATED_RES})
    target_link_libraries(sandbox_binpacks hut_imgui_impl)

    add_executable(sandbox_text tst/sandbox_text_imgui.cpp ${HUT_DEMO_GENERATED_RES})
    target_link_libraries(sandbox_text hut_imgui_impl)

    add_executable(sandbox_emoji tst/sandbox_emoji_imgui.cpp ${HUT_DEMO_GENERATED_RES})
    target_link_libraries(sandbox_emoji hut_imgui_impl)

    add_executable(sandbox_atlas tst/sandbox_atlas_imgui.cpp ${HUT_DEMO_GENERATED_RES})
    target_link_libraries(sandbox_atlas hut_imgui_impl)
  endif()
endif()

###########################################################
# Unit tests
###########################################################
if(GTEST_FOUND)
  file(GLOB HUT_UNIT_TESTS tst/unit/*.cpp)
  set(HUT_UNIT_TESTS_DEPS hut ${GTEST_BOTH_LIBRARIES})

  if (HUT_ENABLE_KTX2)
    file(GLOB_RECURSE HUT_KTX2_UNIT_TESTS_DATA tst/ktx2/*.ktx2)
    add_custom_command(OUTPUT ${HUT_GEN_DIR}/unittests_ktx2.cpp
        COMMAND $<TARGET_FILE:gen_res> unittests_ktx2 ${HUT_GEN_DIR} ${HUT_KTX2_UNIT_TESTS_DATA}
        DEPENDS gen_res ${HUT_KTX2_UNIT_TESTS_DATA}
        COMMENT "Generating KTX2 unittests data")

    file(GLOB_RECURSE HUT_KTX2_UNIT_TESTS tst/unit/ktx2/*.cpp)
    set(HUT_UNIT_TESTS ${HUT_UNIT_TESTS} ${HUT_KTX2_UNIT_TESTS} ${HUT_GEN_DIR}/unittests_ktx2.cpp)
    set(HUT_UNIT_TESTS_DEPS ${HUT_UNIT_TESTS_DEPS} hut_ktx2)
  endif()

  if (HUT_ENABLE_RENDERDOC)
    set(HUT_UNIT_TESTS_DEPS ${HUT_UNIT_TESTS_DEPS} hut_renderdoc)
  endif()

  add_executable(hut_unittests ${HUT_UNIT_TESTS})
  add_test(NAME hut_unittests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND unittests)
  target_link_libraries(hut_unittests ${HUT_UNIT_TESTS_DEPS})
endif()
