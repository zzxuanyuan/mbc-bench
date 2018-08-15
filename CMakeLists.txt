cmake_minimum_required(VERSION 2.6)

set(MBC_PROJECT_NAME
	mbc
)

project(${MBC_PROJECT_NAME})

set(MBC_SOURCE_DIR
	${PROJECT_SOURCE_DIR}/src
)

set(MBC_SOURCE
	${MBC_SOURCE_DIR}/main.cpp
)

set(MBC_TEST_DIR
	${PROJECT_SOURCE_DIR}/test
)

set(MBC_LIBRARIES_DIR
	${PROJECT_SOURCE_DIR}/lib
)

set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin )

# google test #
add_subdirectory(lib/googletest)
include_directories(${googletest-distribution_SOURCE_DIR}/include ${googletest-distribution_SOURCE_DIR})

find_library(gtest
       NAMES gtest
       PATHS ${googletest-distribution_BINARY_DIR}
       NO_DEFAULT_PATH
)

add_subdirectory(src)
add_subdirectory(test)
