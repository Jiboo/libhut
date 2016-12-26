# - Try to find GLM
# Once done this will define
#  GLM_FOUND - System has LibXml2
#  GLM_INCLUDE_DIRS - The LibXml2 include directories
#  GLM_DEFINITIONS - Compiler switches required for using LibXml2

find_path(GLM_INCLUDE_DIR glm/glm.hpp)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GLM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GLM DEFAULT_MSG
    GLM_INCLUDE_DIR)

mark_as_advanced(GLM_INCLUDE_DIR GLM_LIBRARY)

set(GLM_LIBRARIES ${GLM_LIBRARY})
set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
