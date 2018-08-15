# mbc-bench (Multi-block compression benchmark)

This library is designed to compare compression metrics among three compression techniques: SBC(single-block compression); MBC(multi-block compression); RAC(random access compression).

## Building the package

Prerequisite: CMake 2.6.0

Dependencies: lz4, zstd, gtest.

Build instructions:

1. Run do_cmake.sh script
or
2. Follow the following steps
	mkdir build
	cd build
	cmake ..
	make
and this assumes you make your build directory as a subdirectory of the mbc-bench project directory.

## Executable

The program should be compiled and stored in bin directory as well as the project source directory. The executable name is mbc-bench.

## Unit tests

All unit tests will be compiled to bin directory and named as test_*.