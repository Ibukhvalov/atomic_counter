cmake_minimum_required(VERSION 3.20)

project(atomic_counter VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(atomic_counter INTERFACE)
add_library(atomic_counter::atomic_counter ALIAS atomic_counter)

target_compile_features(atomic_counter INTERFACE cxx_std_17)
target_include_directories(atomic_counter
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(atomic_counter INTERFACE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(atomic_counter INTERFACE /W4 /permissive-)
endif()

if(BUILD_TESTING)
    find_package(Threads REQUIRED)

    add_executable(atomic_counter_tests test_atomic_counter.cpp)
    target_link_libraries(atomic_counter_tests
        PRIVATE
            atomic_counter::atomic_counter
            Threads::Threads)

    add_test(NAME atomic_counter.tests COMMAND atomic_counter_tests)
endif()

install(
    FILES atomic_counter.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
    TARGETS atomic_counter
    EXPORT atomic_counterTargets)

install(
    EXPORT atomic_counterTargets
    FILE atomic_counterTargets.cmake
    NAMESPACE atomic_counter::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/atomic_counter)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/atomic_counterConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/atomic_counterConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/atomic_counter)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/atomic_counterConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/atomic_counterConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/atomic_counterConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/atomic_counter)
