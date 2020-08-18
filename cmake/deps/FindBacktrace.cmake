# - Try to find libbacktrace
# Once done, this will define
#
#   BACKTRACE_FOUND - System has XKBCommon
#   BACKTRACE_INCLUDE_DIRS - The XKBCommon include directories
#   BACKTRACE_LIBRARIES - The libraries needed to use XKBCommon
#   BACKTRACE_DEFINITIONS - Compiler switches required for using XKBCommon

find_path(BACKTRACE_INCLUDE_DIR NAMES backtrace.h )
find_library(BACKTRACE_LIBRARY NAMES backtrace)

set(BACKTRACE_LIBRARIES ${BACKTRACE_LIBRARY})
set(BACKTRACE_LIBRARY_DIRS ${BACKTRACE_LIBRARY_DIRS})
set(BACKTRACE_INCLUDE_DIRS ${BACKTRACE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Backtrace DEFAULT_MSG BACKTRACE_LIBRARY BACKTRACE_INCLUDE_DIR)

mark_as_advanced(BACKTRACE_LIBRARY BACKTRACE_INCLUDE_DIR)
