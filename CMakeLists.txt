cmake_minimum_required(VERSION 3.10)
project(dynamicxx VERSION 0.1.0)

# --- Library ---
# As a header-only library, we just need to tell CMake where to find the headers.
add_library(dynamicxx INTERFACE)
target_include_directories(dynamicxx INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(dynamicxx INTERFACE cxx_std_11)

# --- Dependencies ---
# Add googletest
find_package(googletest QUIET)
if (NOT googletest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG    v1.17.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif ()

# --- Examples ---
add_executable(example examples/main.cc)
target_link_libraries(example PRIVATE dynamicxx)

# --- Tests ---
add_executable(run_tests tests/main.cc)
target_link_libraries(run_tests PRIVATE dynamicxx gtest_main)

include(GoogleTest)
gtest_discover_tests(run_tests)

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
