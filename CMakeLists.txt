cmake_minimum_required(VERSION 3.10)
project(dynamicxx VERSION 0.1.0)

option(DYNAMICXX_BUILD_TESTS "Build the tests for dynamicxx" OFF)
option(DYNAMICXX_BUILD_EXAMPLES "Build the examples for dynamicxx" OFF)

# --- Library ---
# As a header-only library, we just need to tell CMake where to find the headers.
add_library(dynamicxx INTERFACE)
target_include_directories(dynamicxx INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(dynamicxx INTERFACE cxx_std_11)

# --- Installation ---
include(GNUInstallDirs)

install(
  DIRECTORY include/dynamicxx
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  TARGETS dynamicxx
  EXPORT dynamicxx-export
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
export(
  EXPORT dynamicxx-export
  NAMESPACE dynamicxx::
  FILE dynamicxx-targets.cmake
)

include(CMakePackageConfigHelpers)

# Generate the version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/dynamicxx-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Generate the config file
configure_package_config_file(
  "cmake/dynamicxx-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dynamicxx-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dynamicxx
)

# Install the config and version files
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dynamicxx-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dynamicxx-config-version.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/dynamicxx
)

if (DYNAMICXX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()

if (DYNAMICXX_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()
