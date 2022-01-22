# https://github.com/LunarG/VulkanSamples/blob/master/cmake/FindWayland.cmake

# Try to find Wayland on a Unix system
#
# This will define:
#
#   WAYLAND_FOUND       - True if Wayland is found
#   WAYLAND_LIBRARIES   - Link these to use Wayland
#   WAYLAND_INCLUDE_DIR - Include directory for Wayland
#   WAYLAND_DEFINITIONS - Compiler flags for using Wayland
#
# In addition the following more fine grained variables will be defined:
#
#   WAYLAND_CLIENT_FOUND  WAYLAND_CLIENT_INCLUDE_DIR  WAYLAND_CLIENT_LIBRARIES
#   WAYLAND_SERVER_FOUND  WAYLAND_SERVER_INCLUDE_DIR  WAYLAND_SERVER_LIBRARIES
#   WAYLAND_EGL_FOUND     WAYLAND_EGL_INCLUDE_DIR     WAYLAND_EGL_LIBRARIES
#
# Copyright (c) 2013 Martin Gräßlin <mgraesslin@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(PkgConfig)

if (NOT Wayland_FIND_COMPONENTS)
  set(Wayland_FIND_COMPONENTS wayland-client)
endif ()

include(FindPackageHandleStandardArgs)
set(WAYLAND_FOUND TRUE)
set(WAYLAND_INCLUDE_DIRS "")
set(WAYLAND_LIBRARIES "")

foreach (comp ${Wayland_FIND_COMPONENTS})
  string(TOUPPER ${comp} compname)
  string(REPLACE "-" "_" compname ${compname})
  set(libname ${comp})
  set(headername ${comp}.h)

  #message("searching for ${comp}: ${comp} and ${libname}")
  pkg_check_modules(PC_${comp} ${comp})

  if (NOT PC_${comp}_FOUND)
    message("Couldn't find wayland component: ${comp}")
    set(WAYLAND_FOUND FALSE)
  else ()
    find_path(${compname}_INCLUDE_DIR NAMES ${headername}
        HINTS
        ${PC_${comp}_INCLUDEDIR}
        ${PC_${comp}_INCLUDE_DIRS}
        )
    list(APPEND WAYLAND_INCLUDE_DIRS ${${compname}_INCLUDE_DIR})

    find_library(${compname}_LIBRARY NAMES ${libname}
        HINTS
        ${PC_${comp}_LIBDIR}
        ${PC_${comp}_LIBRARY_DIRS}
        )
    list(APPEND WAYLAND_LIBRARIES ${${compname}_LIBRARY})

    #message("${comp} results: ${${compname}_INCLUDE_DIR} ${${compname}_LIBRARY}")
  endif ()
endforeach ()

#message("FindWayland results: ${WAYLAND_FOUND} ${WAYLAND_INCLUDE_DIRS} ${WAYLAND_LIBRARIES}")

find_package_handle_standard_args(Wayland
    FOUND_VAR WAYLAND_FOUND
    REQUIRED_VARS WAYLAND_INCLUDE_DIRS WAYLAND_LIBRARIES)
mark_as_advanced(WAYLAND_INCLUDE_DIRS WAYLAND_LIBRARIES)
