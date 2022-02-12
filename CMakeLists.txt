cmake_minimum_required(VERSION 3.20)
project(hut)
enable_testing()

###########################################################
# Configuration
###########################################################
option(HUT_ENABLE_IMGDEC "Enable the image loading extension" TRUE)
option(HUT_ENABLE_IMGUI "Enable the imgui extension, which is used for some tests and playgrounds" TRUE)
option(HUT_ENABLE_KTX2 "Enable the ktx2 io extension library" TRUE)
option(HUT_ENABLE_RENDER2D "Enable the imgui extension, which is used for some tests and playgrounds" TRUE)
option(HUT_ENABLE_RENDERDOC "Enable the renderdoc extension allowing to trigger captures from code" FALSE)
option(HUT_ENABLE_TEXT "Enable the text extension allowing to raster glyphs with freetype and layout text with harfbuzz" TRUE)
option(HUT_ENABLE_UI "Enable the text extension for UI, requires the render2d and text extensions" TRUE)

option(HUT_COMPILE_SAMPLES "Compile all samples/demos, can take some time due to shader/resources pipelines" TRUE)
option(HUT_COMPILE_PLAYGROUNDS "Compile all test playgrounds" TRUE)
option(HUT_COMPILE_UNITTESTS "Compile all unit tests" TRUE)

###########################################################
# Debug
###########################################################
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_PROFILING)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_VALIDATION_DEBUG)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_PROFILE_BOOT)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_TIME_EVENTS)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_DEBUG_STAGING)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_DEBUG_WL_DATA_LISTENERS)
#set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_PREFER_NONDESCRETE_DEVICES)
#set(GLSLANG_VALIDATOR_FLAGS "-g")

###########################################################
# Compile/link flags
###########################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_MODE HIGH)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_UNITY_BUILD ON)
#set(CMAKE_UNITY_BUILD_BATCH_SIZE 0)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")

###########################################################
# Checking for required dependencies...
###########################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps/")

include(dep/spng.cmake)
include(dep/imgui.cmake)
include(dep/spirv_reflect.cmake)

find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)

find_package(Threads REQUIRED)
set(HUT_LIBS ${HUT_LIBS} ${CMAKE_THREAD_LIBS_INIT})

find_package(Vulkan REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${Vulkan_INCLUDE_DIRS})
set(HUT_LIBS ${HUT_LIBS} ${Vulkan_LIBRARIES})

find_package(GLM REQUIRED)
set(HUT_INCLUDES ${HUT_INCLUDES} ${GLM_INCLUDE_DIRS})
set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DGLM_FORCE_LEFT_HANDED)

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
endif ()
if (NOT WAYLAND_SCANNER)
  message(FATAL_ERROR "Couldn't locate wayland-scanner, might need to install")
endif ()

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
find_package(GTest)

string(FIND "${HUT_DEFINITIONS}" "HUT_ENABLE_PROFILING" HUT_ENABLE_PROFILING)
if (NOT HUT_ENABLE_PROFILING EQUAL -1)
  find_package(fmt REQUIRED) #TODO Replace with std::format
  find_package(Boost REQUIRED)
  find_package(Backtrace) #TODO Replace with std::stacktrace

  set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DHUT_ENABLE_PROFILING)
  set(HUT_INCLUDES ${HUT_INCLUDES} ${Boost_INCLUDE_DIRS})
  set(HUT_LIBS ${HUT_LIBS} fmt::fmt-header-only)
  set(HUT_LIBS ${HUT_LIBS} dl) # Dependency of boost-stacktrace

  if (Backtrace_FOUND)
    set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DBOOST_STACKTRACE_USE_BACKTRACE)
    set(HUT_INCLUDES ${HUT_INCLUDES} ${BACKTRACE_INCLUDE_DIRS})
    set(HUT_LIBS ${HUT_LIBS} ${BACKTRACE_LIBRARIES})
  else ()
    message(WARNING "Defaulting to addr2line, which is slow to generate backtraces, please install https://github.com/ianlancetaylor/libbacktrace")
    set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DBOOST_STACKTRACE_USE_ADDR2LINE)
  endif ()
endif ()

###########################################################
# Shader generation helpers
###########################################################
function(hut_gen_spv_bundle)
  cmake_parse_arguments(HUT_GEN_SPV_BUNDLE "" "NAMESPACE;OUTDIR" "SHADER_SOURCES" ${ARGN})
  if (NOT HUT_GEN_SPV_BUNDLE_NAMESPACE)
    message(FATAL_ERROR "No namespace specified for hut_gen_spv_bundle")
  elseif (NOT HUT_GEN_SPV_BUNDLE_OUTDIR)
    message(FATAL_ERROR "No output directory specified for hut_gen_spv_bundle")
  elseif (NOT HUT_GEN_SPV_BUNDLE_SHADER_SOURCES)
    message(FATAL_ERROR "No shader source list provided for hut_gen_spv_bundle")
  endif ()

  file(MAKE_DIRECTORY ${HUT_GEN_SPV_BUNDLE_OUTDIR})

  foreach (shader_source ${HUT_GEN_SPV_BUNDLE_SHADER_SOURCES})
    get_filename_component(shader_target ${shader_source} NAME)
    string(REPLACE ".frag" ".frag.spv" shader_target "${shader_target}")
    string(REPLACE ".vert" ".vert.spv" shader_target "${shader_target}")
    set(shader_target ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${shader_target})
    set(HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS};${shader_target})

    add_custom_command(OUTPUT ${shader_target}
        COMMAND ${GLSLANG_VALIDATOR} ${GLSLANG_VALIDATOR_FLAGS} -V ${shader_source} -o ${shader_target}
        DEPENDS ${shader_source} ${GLSLANG_VALIDATOR}
        COMMENT "Generating SPIR-V for ${shader_source}")
  endforeach ()

  add_custom_command(OUTPUT ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}.hpp
      COMMAND $<TARGET_FILE:gen_res> ${HUT_GEN_SPV_BUNDLE_NAMESPACE} ${HUT_GEN_SPV_BUNDLE_OUTDIR} ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
      DEPENDS gen_res ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
      COMMENT "Generating ${HUT_GEN_SPV_BUNDLE_NAMESPACE}.cpp/hpp")

  add_custom_command(OUTPUT ${HUT_GEN_SPV_BUNDLE_OUTDIR}/${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl.hpp
      COMMAND $<TARGET_FILE:gen_spv> ${HUT_GEN_SPV_BUNDLE_NAMESPACE}_refl ${HUT_GEN_SPV_BUNDLE_OUTDIR} ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
      DEPENDS gen_spv ${HUT_GEN_SPV_BUNDLE_SHADER_OBJECTS}
      COMMENT "Generating SPIR-V reflection for ${HUT_GEN_SPV_BUNDLE_NAMESPACE}")

  set(HUT_BUNDLE_${HUT_GEN_SPV_BUNDLE_NAMESPACE}_FILES
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
set(HUT_DEFINITIONS ${HUT_DEFINITIONS} -DVK_USE_PLATFORM_WAYLAND_KHR)
file(GLOB_RECURSE HUT_HEADERS inc/hut/*.hpp inc/hut/utils/*.hpp)
file(GLOB HUT_COMMON_SOURCES src/*.cpp)
set(HUT_SOURCES ${HUT_SOURCES} ${HUT_COMMON_SOURCES})

set(HUT_INCLUDES ${HUT_INCLUDES} inc)
add_library(hut STATIC ${HUT_HEADERS} ${HUT_SOURCES})
target_link_libraries(hut ${HUT_LIBS})
target_compile_definitions(hut PUBLIC ${HUT_DEFINITIONS})
target_include_directories(hut PUBLIC ${HUT_INCLUDES})
target_precompile_headers(hut PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/inc/hut/utils/precomp.hpp>")

#message("HUT_DEFINITIONS: ${HUT_DEFINITIONS}")
#message("HUT_INCLUDES: ${HUT_INCLUDES}")
#message("HUT_LIBS: ${HUT_LIBS}")
#message("HUT_SOURCES: ${HUT_SOURCES}")

###########################################################
# hut extensions
###########################################################
function(hut_add_extension)
  cmake_parse_arguments(HUT_ADD_EXTENSION "" "NAME" "DEPENDENCIES;EXTRA_LIBS" ${ARGN})
  if (NOT HUT_ADD_EXTENSION_NAME)
    message(FATAL_ERROR "No name specified for hut_add_extension")
  endif ()
  string(TOUPPER ${HUT_ADD_EXTENSION_NAME} HUT_ADD_EXTENSION_NAME_UPPER)

  if (HUT_ENABLE_${HUT_ADD_EXTENSION_NAME_UPPER})
    set(EXT_INCLUDE_DIR ext/${HUT_ADD_EXTENSION_NAME}/inc)
    set(EXT_SOURCE_DIR ext/${HUT_ADD_EXTENSION_NAME}/src)
    set(EXT_SHADER_DIR ext/${HUT_ADD_EXTENSION_NAME}/spv)
    set(EXT_UNITTESTS_DIR ext/${HUT_ADD_EXTENSION_NAME}/tst)
    set(EXT_GEN_DIR ${CMAKE_BINARY_DIR}/gen/${HUT_ADD_EXTENSION_NAME})
    set(EXT_TARGET hut_${HUT_ADD_EXTENSION_NAME})

    file(GLOB_RECURSE EXT_HEADERS ${EXT_INCLUDE_DIR}/*.hpp)
    file(GLOB_RECURSE EXT_SOURCES ${EXT_SOURCE_DIR}/*.cpp)
    file(GLOB_RECURSE EXT_SHADERS ${EXT_SHADER_DIR}/*.vert ${EXT_SHADER_DIR}/*.frag)

    set(EXT_DEP_INCLUDES "")
    set(EXT_DEP_LIBRARIES ${HUT_ADD_EXTENSION_EXTRA_LIBS})
    set(EXT_DEP_TARGETS "")
    foreach (EXT_DEPENDENCY ${HUT_ADD_EXTENSION_DEPENDENCIES})
      if (NOT TARGET ${EXT_DEPENDENCY})
        find_package(${EXT_DEPENDENCY} REQUIRED)
        if (TARGET ${EXT_DEPENDENCY})
          set(EXT_DEP_TARGETS ${EXT_DEP_TARGETS} ${EXT_DEPENDENCY})
        else ()
          string(TOUPPER ${EXT_DEPENDENCY} EXT_DEPENDENCY_UPPER)
          set(EXT_DEP_INCLUDES ${EXT_DEP_INCLUDES} ${${EXT_DEPENDENCY_UPPER}_INCLUDE_DIRS})
          set(EXT_DEP_LIBRARIES ${EXT_DEP_LIBRARIES} ${${EXT_DEPENDENCY_UPPER}_LIBRARIES})
        endif ()
      else ()
        set(EXT_DEP_TARGETS ${EXT_DEP_TARGETS} ${EXT_DEPENDENCY})
      endif ()
    endforeach ()

    if (EXT_SHADERS)
      hut_gen_spv_bundle(NAMESPACE ${HUT_ADD_EXTENSION_NAME} OUTDIR ${EXT_GEN_DIR} SHADER_SOURCES ${EXT_SHADERS})
    else ()
      set(HUT_BUNDLE_${HUT_ADD_EXTENSION_NAME}_FILES "")
    endif ()

    add_library(${EXT_TARGET} STATIC ${EXT_HEADERS} ${EXT_SOURCES} ${HUT_BUNDLE_${HUT_ADD_EXTENSION_NAME}_FILES})
    target_link_libraries(${EXT_TARGET} hut ${EXT_DEP_TARGETS} ${EXT_DEP_LIBRARIES})
    target_include_directories(${EXT_TARGET} PUBLIC ${EXT_INCLUDE_DIR} ${EXT_GEN_DIR} ${EXT_DEP_INCLUDES})
    target_compile_definitions(${EXT_TARGET} PUBLIC -DHUT_ENABLE_${HUT_ADD_EXTENSION_NAME_UPPER})

    if (HUT_COMPILE_UNITTESTS)
      file(GLOB_RECURSE EXT_UNIT_TESTS ${EXT_UNITTESTS_DIR}/*.cpp)
      if (EXT_UNIT_TESTS)
        file(GLOB_RECURSE EXT_TST_DATA ${EXT_UNITTESTS_DIR}/res/*.*)
        if (EXT_TST_DATA)
          add_custom_command(OUTPUT ${EXT_GEN_DIR}/tst_data.cpp
              COMMAND $<TARGET_FILE:gen_res> tst_data ${EXT_GEN_DIR} ${EXT_TST_DATA}
              DEPENDS gen_res ${EXT_TST_DATA}
              COMMENT "Generating ${HUT_ADD_EXTENSION_NAME} unittests data")
          set(EXT_UNIT_TESTS ${EXT_UNIT_TESTS} ${EXT_GEN_DIR}/tst_data.cpp)
        endif ()
        set(HUT_UNIT_TESTS_DEPS ${HUT_UNIT_TESTS_DEPS} ${EXT_TARGET} PARENT_SCOPE)
        set(HUT_UNIT_TESTS ${HUT_UNIT_TESTS} ${EXT_UNIT_TESTS} PARENT_SCOPE)
      endif ()
    endif ()
  endif ()
endfunction()

hut_add_extension(NAME imgdec DEPENDENCIES ZLIB SimplePNG)
hut_add_extension(NAME imgui DEPENDENCIES DearImGui)
hut_add_extension(NAME ktx2)
hut_add_extension(NAME render2d)
hut_add_extension(NAME renderdoc DEPENDENCIES Renderdoc EXTRA_LIBS ${CMAKE_DL_LIBS})
hut_add_extension(NAME text DEPENDENCIES Freetype HarfBuzz)
hut_add_extension(NAME ui DEPENDENCIES hut_render2d hut_text)

###########################################################
# hut test data
###########################################################
set(HUT_TST_GEN_DIR ${CMAKE_BINARY_DIR}/gen/tst)

file(GLOB HUT_TST_PNG_SOURCE tst/res/png/*.png)
set(HUT_TST_PNG ${HUT_TST_GEN_DIR}/png/tst_png.cpp)
add_custom_command(OUTPUT ${HUT_TST_PNG}
    COMMAND $<TARGET_FILE:gen_res> tst_png ${HUT_TST_GEN_DIR}/png ${HUT_TST_PNG_SOURCE}
    DEPENDS gen_res ${HUT_TST_PNG_SOURCE}
    COMMENT "Generating ${HUT_TST_PNG}")
add_library(hut_tst_data_png ${HUT_TST_PNG})
target_link_libraries(hut_tst_data_png hut)
target_include_directories(hut_tst_data_png PUBLIC ${HUT_TST_GEN_DIR}/png)

file(GLOB_RECURSE HUT_TST_WOFF2_SOURCE tst/res/woff2/*.woff2)
set(HUT_TST_WOFF2 ${HUT_TST_GEN_DIR}/woff2/tst_woff2.cpp)
add_custom_command(OUTPUT ${HUT_TST_WOFF2}
    COMMAND $<TARGET_FILE:gen_res> tst_woff2 ${HUT_TST_GEN_DIR}/woff2 ${HUT_TST_WOFF2_SOURCE}
    DEPENDS gen_res ${HUT_TST_WOFF2_SOURCE}
    COMMENT "Generating ${HUT_TST_WOFF2}")
add_library(hut_tst_data_woff2 ${HUT_TST_WOFF2})
target_link_libraries(hut_tst_data_woff2 hut)
target_include_directories(hut_tst_data_woff2 PUBLIC ${HUT_TST_GEN_DIR}/woff2)

file(GLOB HUT_TST_SHADERS_SOURCE tst/res/spv/*.vert tst/res/spv/*.frag)
hut_gen_spv_bundle(NAMESPACE tst_shaders OUTDIR ${HUT_TST_GEN_DIR}/shaders SHADER_SOURCES ${HUT_TST_SHADERS_SOURCE})
add_library(hut_tst_data_shaders ${HUT_BUNDLE_tst_shaders_FILES})
target_link_libraries(hut_tst_data_shaders hut)
target_include_directories(hut_tst_data_shaders PUBLIC ${HUT_TST_GEN_DIR}/shaders)

###########################################################
# hut tests
###########################################################
function(hut_add_test)
  cmake_parse_arguments(HUT_ADD_TEST "" "NAME;PATH" "DEPENDENCIES" ${ARGN})
  if (NOT HUT_ADD_TEST_NAME)
    message(FATAL_ERROR "No name specified for hut_add_test")
  endif ()
  if (NOT HUT_ADD_TEST_PATH)
    message(FATAL_ERROR "No path specified for hut_add_test")
  endif ()

  set(TST_MISSING_DEPENDENCIES "")
  foreach (EXT_DEPENDENCY ${HUT_ADD_TEST_DEPENDENCIES})
    if (NOT TARGET ${EXT_DEPENDENCY})
      set(TST_MISSING_DEPENDENCIES ${TST_MISSING_DEPENDENCIES} ${EXT_DEPENDENCY})
    endif ()
  endforeach ()

  if (NOT TST_MISSING_DEPENDENCIES)
    add_executable(${HUT_ADD_TEST_NAME} ${HUT_ADD_TEST_PATH})
    target_link_libraries(${HUT_ADD_TEST_NAME} hut ${HUT_ADD_TEST_DEPENDENCIES})
    target_include_directories(${HUT_ADD_TEST_NAME} PRIVATE tst/common)
  else ()
    message("Disabling test ${HUT_ADD_TEST_NAME} due to missing dependencies: ${TST_MISSING_DEPENDENCIES}")
  endif ()
endfunction()

if (HUT_COMPILE_SAMPLES)
  hut_add_test(NAME hut_sample_3d PATH tst/samples/sample_3d.cpp DEPENDENCIES hut_tst_data_shaders)
  hut_add_test(NAME hut_sample_imgui PATH tst/samples/sample_imgui.cpp DEPENDENCIES hut_imgui)
endif ()

if (HUT_COMPILE_PLAYGROUNDS)
  hut_add_test(NAME hut_playground_events PATH tst/playgrounds/playground_events.cpp DEPENDENCIES hut_imgui)
  hut_add_test(NAME hut_playground_binpacks PATH tst/playgrounds/playground_binpacks.cpp DEPENDENCIES hut_imgui)
  hut_add_test(NAME hut_playground_dragndrop PATH tst/playgrounds/playground_dragndrop.cpp DEPENDENCIES hut_imgui)
  hut_add_test(NAME hut_playground_window PATH tst/playgrounds/playground_window.cpp DEPENDENCIES hut_imgui)
  hut_add_test(NAME hut_playground_clipboard PATH tst/playgrounds/playground_clipboard.cpp DEPENDENCIES hut_imgui hut_tst_data_png)
  hut_add_test(NAME hut_playground_text PATH tst/playgrounds/playground_text.cpp DEPENDENCIES hut_imgui hut_text hut_tst_data_woff2 hut_tst_data_shaders)
  hut_add_test(NAME hut_playground_render2d PATH tst/playgrounds/playground_render2d.cpp DEPENDENCIES hut_render2d hut_imgui hut_imgdec hut_tst_data_png)
  hut_add_test(NAME hut_playground_ui PATH tst/playgrounds/playground_ui.cpp DEPENDENCIES hut_ui hut_text)
endif ()

if (HUT_COMPILE_UNITTESTS AND GTEST_FOUND)
  file(GLOB_RECURSE HUT_TST_UNIT_SOURCE tst/unit/*.cpp)

  set(HUT_UNIT_TESTS ${HUT_UNIT_TESTS} ${HUT_TST_UNIT_SOURCE})
  set(HUT_UNIT_TESTS_DEPS ${HUT_UNIT_TESTS_DEPS} ${GTEST_BOTH_LIBRARIES})

  if (HUT_ENABLE_RENDERDOC)
    set(HUT_UNIT_TESTS_DEPS ${HUT_UNIT_TESTS_DEPS} hut_renderdoc)
  endif ()

  add_executable(hut_unittests ${HUT_UNIT_TESTS} ${HUT_TST_SHADERS})
  add_test(NAME hut_unittests WORKING_DIRECTORY COMMAND hut_unittests)
  target_link_libraries(hut_unittests ${HUT_UNIT_TESTS_DEPS} hut_tst_data_shaders)
  target_include_directories(hut_unittests PRIVATE tst/common)
endif ()
