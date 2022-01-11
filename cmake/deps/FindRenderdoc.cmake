# - Try to find librenderdoc
# Once done, this will define
#
#   RENDERDOC_FOUND - System has Renderdoc
#   RENDERDOC_INCLUDE_DIRS - The Renderdoc include directories
#   RENDERDOC_LIBRARIES - The libraries needed to use Renderdoc
#   RENDERDOC_DEFINITIONS - Compiler switches required for using Renderdoc

find_path(RENDERDOC_INCLUDE_DIR NAMES renderdoc_app.h)
find_library(RENDERDOC_LIBRARY NAMES renderdoc)

set(RENDERDOC_LIBRARIES ${RENDERDOC_LIBRARY})
set(RENDERDOC_INCLUDE_DIRS ${RENDERDOC_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Renderdoc DEFAULT_MSG RENDERDOC_LIBRARY RENDERDOC_INCLUDE_DIR)

mark_as_advanced(RENDERDOC_LIBRARY RENDERDOC_INCLUDE_DIR)
