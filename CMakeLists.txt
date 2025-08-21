cmake_minimum_required(VERSION 3.20)
project(emsenv VERSION 0.1.0 LANGUAGES C)

add_library(emsenv src/lib.c)
set_target_properties(emsenv PROPERTIES LINKER_LANGUAGE C)
add_library(emsenv::emsenv ALIAS emsenv)

include(GNUInstallDirs)

set(emsenv_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/emsenv")

install(
  TARGETS emsenv
  EXPORT  emsenvTargets
)

install(
  EXPORT      emsenvTargets
  FILE        emsenvTargets.cmake
  NAMESPACE   emsenv::
  DESTINATION "${emsenv_INSTALL_CMAKEDIR}"
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/emsenvConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/emsenvConfig.cmake"
  INSTALL_DESTINATION "${emsenv_INSTALL_CMAKEDIR}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/emsenvConfigVersion.cmake"
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/emsenvConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/emsenvConfigVersion.cmake"
  DESTINATION "${emsenv_INSTALL_CMAKEDIR}"
)
