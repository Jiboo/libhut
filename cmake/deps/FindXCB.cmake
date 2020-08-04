# - FindXCB
#
# Copyright 2015 Valve Coporation

find_package(PkgConfig)

if (NOT XCB_FIND_COMPONENTS)
  set(XCB_FIND_COMPONENTS xcb)
endif ()

include(FindPackageHandleStandardArgs)
set(XCB_FOUND true)
set(XCB_INCLUDE_DIRS "")
set(XCB_LIBRARIES "")
foreach (comp ${XCB_FIND_COMPONENTS})
  # component name
  string(TOUPPER ${comp} compname)
  string(REPLACE "-" "_" compname ${compname})

  set(libname ${comp})
  string(REPLACE "-" "_" headername xcb/${comp}.h)

  message("searching for ${comp}: ${headername} and ${libname}")
  pkg_check_modules(PC_${comp} ${comp})

  if(NOT PC_${comp}_FOUND)
    message("Couldn't find xcb component: ${comp}")
    set(XCB_FOUND false)
  else()
    find_path(${compname}_INCLUDE_DIR NAMES ${headername}
        HINTS
        ${PC_${comp}_INCLUDEDIR}
        ${PC_${comp}_INCLUDE_DIRS}
        )

    find_library(${compname}_LIBRARY NAMES ${libname}
        HINTS
        ${PC_${comp}_LIBDIR}
        ${PC_${comp}_LIBRARY_DIRS}
        )

    list(APPEND XCB_INCLUDE_DIRS ${${compname}_INCLUDE_DIR})
    list(APPEND XCB_LIBRARIES ${${compname}_LIBRARY})
  endif()
endforeach ()

find_package_handle_standard_args(XCB
        FOUND_VAR XCB_FOUND
        REQUIRED_VARS XCB_INCLUDE_DIRS XCB_LIBRARIES)
mark_as_advanced(XCB_INCLUDE_DIRS XCB_LIBRARIES)

list(REMOVE_DUPLICATES XCB_INCLUDE_DIRS)
