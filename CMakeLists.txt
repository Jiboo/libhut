cmake_minimum_required(VERSION 3.0.0)
project(libhut)

enable_testing()
find_package(GTest REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wno-missing-braces -Wno-unused-parameter -DHUT_WAYLAND")

set(HUT_HEADERS
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
)

set(EGL_HEADERS
    inc/libhut/egl/display.hpp
)

set(EGL_SOURCES
    src/egl/display.cpp
)

set(WAYLAND_HEADERS
    inc/libhut/wayland/application.hpp
    inc/libhut/wayland/wayland.hpp
)

set(WAYLAND_SOURCES
    ${EGL_SOURCES}
    src/wayland/xdg-shell-protocol.c

    src/wayland/application.cpp
    src/wayland/display.cpp
    src/wayland/display_keyboard.cpp
    src/wayland/display_pointer.cpp
    src/wayland/display_touch.cpp
    src/wayland/window.cpp
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
include_directories(/usr/include)

add_library(hut STATIC ${HUT_SOURCES} ${WAYLAND_SOURCES})
target_link_libraries(hut PUBLIC wayland-client wayland-egl wayland-cursor xkbcommon EGL GLESv2)

add_executable(unit-tests ${TEST_FILES} ${HUT_HEADERS})
target_link_libraries(unit-tests gtest hut)

add_custom_target(docs COMMAND
    cldoc generate -std=c++1y -Iinc -DHUT_WAYLAND --
    --merge doc
    --output gen
    --report
    --static
    inc/hut.hpp ${HEADERS_FILES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
set_target_properties(docs PROPERTIES
    EXCLUDE_FROM_ALL 1
    EXCLUDE_FROM_DEFAULT_BUILD 1
)
