cmake_minimum_required(VERSION 2.8)
project(libhut)

enable_testing()
find_package(GTest REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -DHUT_WAYLAND")

set(HUT_HEADERS
    inc/hut.hpp
    inc/libhut/vec.hpp
    inc/libhut/mat.hpp
    inc/libhut/utils.hpp
    inc/libhut/color.hpp
    inc/libhut/event.hpp
    inc/libhut/property.hpp
    inc/libhut/application.hpp
    inc/libhut/display.hpp
    inc/libhut/surface.hpp
    inc/libhut/window.hpp
    inc/libhut/buffer.hpp
    inc/libhut/texture.hpp
    inc/libhut/drawable.hpp
    inc/libhut/framebuffer.hpp
)

set(EGL_HEADERS
    ${HUT_HEADERS}

    inc/libhut/egl/application.hpp
    inc/libhut/egl/display.hpp
    inc/libhut/egl/surface.hpp
    inc/libhut/egl/window.hpp
    inc/libhut/egl/drawable.hpp
    inc/libhut/egl/buffer.hpp
    inc/libhut/egl/texture.hpp
    inc/libhut/egl/framebuffer.hpp
)

set(WAYLAND_HEADERS
    ${EGL_HEADERS}

    inc/libhut/wayland/application.hpp
    inc/libhut/wayland/display.hpp
    inc/libhut/wayland/surface.hpp
    inc/libhut/wayland/window.hpp
)

set(HUT_SOURCES
    src/utils.cpp
)

set(EGL_SOURCES
    ${HUT_SOURCES}

    src/egl/display.cpp
    src/egl/drawable.cpp
)

set(WAYLAND_SOURCES
    ${EGL_SOURCES}

    src/wayland/application.cpp
    src/wayland/display.cpp
    src/wayland/display_keyboard.cpp
    src/wayland/display_pointer.cpp
    src/wayland/display_touch.cpp
    src/wayland/window.cpp
    src/wayland/xdg-shell-protocol.c
)

set(TEST_FILES
    tst/main.cpp
    tst/math.cpp
    tst/utils.cpp
)

if(APPLE)
elseif(UNIX)
elseif(WIN32)
endif(APPLE)

include_directories(inc)

add_library(hut STATIC ${WAYLAND_SOURCES} ${WAYLAND_HEADERS})
target_link_libraries(hut PUBLIC wayland-client wayland-egl wayland-cursor xkbcommon EGL GLESv2 png)

add_executable(unit-tests ${TEST_FILES})
target_link_libraries(unit-tests gtest hut)

add_custom_target(docs COMMAND
    cldoc generate -std=c++1y -Iinc -DHUT_WAYLAND -D__CLDOC__ -c --
    --merge doc
    --output doc/gen
    --report
    --static
    ${WAYLAND_SOURCES} inc/hut.hpp
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
set_target_properties(docs PROPERTIES
    EXCLUDE_FROM_ALL 1
    EXCLUDE_FROM_DEFAULT_BUILD 1
)
