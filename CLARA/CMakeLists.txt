## Project
project(CLARA VERSION 0.1.0
              LANGUAGES CXX)

## Options
option(CLARA_TESTING "Build unit tests" ON)
option(CLARA_INSTALL "Install CMake targets" ON)

## Config
include(GNUInstallDirs)
set(CLARA_TARGET_NAME  ${PROJECT_NAME})
set(CLARA_INCLUDE_DIR  "${PROJECT_SOURCE_DIR}/include/")
set(CLARA_SOURCE_DIR   "${PROJECT_SOURCE_DIR}/src/")

## Packages
find_package(fmt CONFIG REQUIRED)

## Target
file(GLOB_RECURSE CLARA_HEADERS CONFIGURE_DEPENDS "${CLARA_INCLUDE_DIR}/*.h")
file(GLOB_RECURSE CLARA_SOURCES CONFIGURE_DEPENDS "${CLARA_SOURCE_DIR}/*.cpp")
add_library(${CLARA_TARGET_NAME} STATIC)
target_sources(${CLARA_TARGET_NAME} PUBLIC ${CLARA_HEADERS} PRIVATE ${CLARA_SOURCES})
target_compile_features(${CLARA_TARGET_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${CLARA_TARGET_NAME} PUBLIC _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
target_include_directories(${CLARA_TARGET_NAME} PUBLIC
	$<BUILD_INTERFACE:${CLARA_INCLUDE_DIR}>
	$<INSTALL_INTERFACE:include>)
target_link_libraries(${CLARA_TARGET_NAME} PRIVATE fmt::fmt)

## Tests
include(CTest)
if(BUILD_TESTING AND CLARA_TESTING)
	enable_testing()
	add_subdirectory(test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
