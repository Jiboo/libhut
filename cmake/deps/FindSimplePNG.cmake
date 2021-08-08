# - Try to find libspng
# Once done, this will define
#
#   SIMPLEPNG_FOUND - System has spng
#   SIMPLEPNG_INCLUDE_DIRS - The spng include directories
#   SIMPLEPNG_LIBRARIES - The libraries needed to use spng
#   SIMPLEPNG_DEFINITIONS - Compiler switches required for using spng

find_path(SIMPLEPNG_INCLUDE_DIR NAMES spng.h)
find_library(SIMPLEPNG_LIBRARY NAMES spng)

set(SIMPLEPNG_LIBRARIES ${SIMPLEPNG_LIBRARY})
set(SIMPLEPNG_INCLUDE_DIRS ${SIMPLEPNG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SimplePNG DEFAULT_MSG SIMPLEPNG_LIBRARY SIMPLEPNG_INCLUDE_DIR)

mark_as_advanced(SIMPLEPNG_LIBRARY SIMPLEPNG_INCLUDE_DIR)
