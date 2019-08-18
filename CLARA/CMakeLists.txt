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
set(CLARA_HEADERS
	"${CLARA_INCLUDE_DIR}/CLARA/Common/Algorithm.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common/File.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common/Imports.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common/Literals.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common/Macros.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common/String.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common/Vector.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Assembly.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Common.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Compiler.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Data.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Diagnostic.h"
	"${CLARA_INCLUDE_DIR}/CLARA/IBinaryOutput.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Label.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Parser.h"
	"${CLARA_INCLUDE_DIR}/CLARA/pch.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Progress.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Reporter.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Source.h"
	"${CLARA_INCLUDE_DIR}/CLARA/System.h"
	"${CLARA_INCLUDE_DIR}/CLARA/Token.h"
	"${CLARA_INCLUDE_DIR}/CLARA/TokenStream.h"
)
set(CLARA_SOURCES
	"${CLARA_SOURCE_DIR}/Common/String.cpp"
	"${CLARA_SOURCE_DIR}/Assembly.cpp"
	"${CLARA_SOURCE_DIR}/Compiler.cpp"
	"${CLARA_SOURCE_DIR}/Parser.cpp"
	"${CLARA_SOURCE_DIR}/pch.cpp"
	"${CLARA_SOURCE_DIR}/Source.cpp"
	"${CLARA_SOURCE_DIR}/Token.cpp"
)
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
