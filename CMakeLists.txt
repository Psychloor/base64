
cmake_minimum_required(VERSION 3.31)
project(base64 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include necessary CMake modules
include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Fetch doctest
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

# Create the main library (header-only)
add_library(base64 INTERFACE)
add_library(${PROJECT_NAME}::base64 ALIAS base64)

target_include_directories(base64 INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Enable testing functionality
if(PROJECT_IS_TOP_LEVEL)
    enable_testing()

    # Create test executable
    add_executable(base64_tests)

    target_sources(base64_tests
            PRIVATE
            ${PROJECT_SOURCE_DIR}/tests/base64_test.cpp
            ${PROJECT_SOURCE_DIR}/tests/base64_test.hpp
    )

    target_include_directories(base64_tests
            PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/tests
    )

    target_link_libraries(base64_tests
            PRIVATE
            doctest::doctest
            base64
    )

    # Register test
    add_test(
            NAME base64_tests
            COMMAND base64_tests
    )

    # Convenience target for running tests
    add_custom_target(run_tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS base64_tests
    )
endif()

# Installation configuration
install(
        TARGETS base64
        EXPORT ${PROJECT_NAME}Targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Generate package version file
write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
)

# Generate the package config file
configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Install package config files
install(
        FILES
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)