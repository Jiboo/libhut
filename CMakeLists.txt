cmake_minimum_required(VERSION 3.16)
project(hut)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_MODE HIGH)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps/")
set(BUILD_SHARED_LIBS OFF) # FIXME JBL: add_library from dependencies makes shared by default without this, leading to missing dlls when launching an EXE because we aren't searching for them in their respective output dir

###########################################################
# Configuration
###########################################################

set(HUT_COMPILE_DEMOS 1)
set(HUT_ENABLE_IMGUI TRUE)

###########################################################
# Debug
###########################################################

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT WIN32)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
set(HUT_ENABLE_PROFILING TRUE)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION_DEBUG)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_DEBUG_STAGING)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_PREFER_NONDESCRETE_DEVICES)
endif()

###########################################################
message("Checking for required dependencies...")
###########################################################

add_subdirectory(dep/glslang)
include(dep/spirv_reflect.cmake)

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

set(GLSLANG_VALIDATOR_FLAGS "")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(GLSLANG_VALIDATOR_FLAGS "${GLSLANG_VALIDATOR_FLAGS} -g")
endif()

###########################################################
message("Checking for optional dependencies...")
###########################################################

if (NOT WIN32 AND HUT_ENABLE_PROFILING)
  find_package(fmt) #TODO Replace with std::format
  find_package(Boost)
  find_package(Backtrace)

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
endif()

find_package(GTest)
if(GTEST_FOUND)
  set(HUT_INCLUDES_TEST ${HUT_INCLUDES} ${GTEST_INCLUDE_DIR})
endif()

find_package(Wayland COMPONENTS wayland-client wayland-cursor)

if(Wayland_FOUND)
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

  set(XDG_DECORATION_PROTOCOL ${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml)
  set(XDG_DECORATION_CLIENT_INCLUDE ${WAYLAND_GEN_DIR}/xdg-decoration-client-protocol-v1.h)
  set(XDG_DECORATION_SOURCE ${WAYLAND_GEN_DIR}/xdg-decoration-protocol-v1.c)
  add_custom_command(
      OUTPUT ${XDG_DECORATION_CLIENT_INCLUDE}
      COMMAND ${WAYLAND_SCANNER} client-header ${XDG_DECORATION_PROTOCOL} ${XDG_DECORATION_CLIENT_INCLUDE}
      DEPENDS ${XDG_DECORATION_PROTOCOL})
  add_custom_command(
      OUTPUT ${XDG_DECORATION_SOURCE}
      COMMAND ${WAYLAND_SCANNER} private-code ${XDG_DECORATION_PROTOCOL} ${XDG_DECORATION_SOURCE}
      DEPENDS ${XDG_DECORATION_PROTOCOL})
  list(APPEND HUT_WAYLAND_GENERATED ${XDG_DECORATION_CLIENT_INCLUDE} ${XDG_DECORATION_SOURCE})

  set(HUT_INCLUDES ${HUT_INCLUDES} ${WAYLAND_GEN_DIR})
  set(HUT_INCLUDES ${HUT_INCLUDES} ${WAYLAND_INCLUDE_DIRS})
  set(HUT_LIBS ${HUT_LIBS} ${WAYLAND_LIBRARIES})
  file(GLOB HUT_WAYLAND_SOURCES src/wayland/*.cpp)
  set(HUT_SOURCES ${HUT_SOURCES} ${HUT_WAYLAND_SOURCES} ${HUT_WAYLAND_GENERATED})
  set(HUT_PLATFORM WAYLAND)
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
            COMMAND $<TARGET_FILE:glslangValidator> ${GLSLANG_VALIDATOR_FLAGS} -V ${shader_source} -o ${shader_target}
            DEPENDS ${shader_source} glslangValidator
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

  add_custom_command(OUTPUT ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl.hpp
          COMMAND $<TARGET_FILE:gen_spv> ${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl ${HUT_GEN_SPV_BUNDLE_OUTDIR} ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
          DEPENDS ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS} gen_spv
          COMMENT "Generating SPIR-V reflection for ${HUT_GEN_SPV_BUNDLE_NAMESPACE}")

  add_custom_target(${bundle_gen_name} DEPENDS ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl.hpp
          COMMENT "Checked if ${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp need regeneration")
endfunction()

add_executable(gen_res src/tools/gen_res/main.cpp)
target_link_libraries(gen_res)
target_include_directories(gen_res PRIVATE inc)

add_executable(gen_spv src/tools/gen_spv/main.cpp)
target_link_libraries(gen_spv spirv_reflect)
target_include_directories(gen_spv PRIVATE inc)

set(HUT_GEN_DIR "${CMAKE_BINARY_DIR}/gen")
file(MAKE_DIRECTORY ${HUT_GEN_DIR})
set(HUT_INCLUDES ${HUT_INCLUDES} ${HUT_GEN_DIR})

file(GLOB HUT_SHADER_SOURCES res/spv/*.frag res/spv/*.vert)

gen_spv_bundle(NAMESPACE hut_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_SHADER_SOURCES})
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_GEN_DIR}/hut_shaders.cpp)

###########################################################
message("Configuring library build targets...")
###########################################################

set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DVK_USE_PLATFORM_${HUT_PLATFORM}_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/hut/*.hpp ${HUT_GEN_DIR}/*.hpp)
file(GLOB HUT_COMMON_SOURCES src/*.cpp ${HUT_GEN_DIR}/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES})

set(HUT_INCLUDES ${HUT_INCLUDES} inc)
add_library(hut STATIC ${HUT_HEADERS} ${HUT_SOURCES})
target_link_libraries(hut ${HUT_LIBS})
target_compile_definitions(hut PUBLIC ${HUT_DEFINITIONS})
target_include_directories(hut PUBLIC ${HUT_INCLUDES})
target_precompile_headers(hut PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/inc/hut/utils.hpp>")
add_dependencies(hut gen_hut_shaders)
message("HUT_DEFINITIONS: ${HUT_DEFINITIONS}")
message("HUT_INCLUDES: ${HUT_INCLUDES}")
message("HUT_LIBS: ${HUT_LIBS}")
message("HUT_SOURCES: ${HUT_SOURCES}")

add_subdirectory(ext)

###########################################################
message("Configuring demo build targets...")
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
  gen_spv_bundle(NAMESPACE demo_shaders OUTDIR ${HUT_GEN_DIR} SHADER_SOURCES ${HUT_DEMO_SHADERS_SOURCE})

  set(HUT_DEMO_GENERATED_RES ${HUT_GEN_DIR}/demo_png.cpp ${HUT_GEN_DIR}/demo_woff2.cpp ${HUT_GEN_DIR}/demo_shaders.cpp)

  add_executable(demo_3d tst/demo_3d.cpp) # try to keep the demo3d "res" free, to use it as gauge for hut size
  target_link_libraries(demo_3d hut)
  add_dependencies(demo_3d gen_demo_shaders)

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
