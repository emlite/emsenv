cmake_minimum_required(VERSION 3.20)
project(ems_env VERSION 0.1.0 LANGUAGES C)

add_library(ems_env src/lib.c)
set_target_properties(ems_env PROPERTIES LINKER_LANGUAGE C)
add_library(ems_env::ems_env ALIAS ems_env)

include(GNUInstallDirs)

set(ems_env_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/ems_env")

install(
  TARGETS ems_env
  EXPORT  ems_envTargets
)

install(
  EXPORT      ems_envTargets
  FILE        ems_envTargets.cmake
  NAMESPACE   ems_env::
  DESTINATION "${ems_env_INSTALL_CMAKEDIR}"
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/ems_envConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ems_envConfig.cmake"
  INSTALL_DESTINATION "${ems_env_INSTALL_CMAKEDIR}"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ems_envConfigVersion.cmake"
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ems_envConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ems_envConfigVersion.cmake"
  DESTINATION "${ems_env_INSTALL_CMAKEDIR}"
)
