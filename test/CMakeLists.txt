set(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(${PROJECT_SOURCE_DIR}/include)

link_directories(${PROJECT_SOURCE_DIR}/lib/lz4/lib ${PROJECT_SOURCE_DIR}/lib/zstd/lib)

add_executable(test_compression
	test_compression.cpp
)

add_executable(test_filer
	test_filer.cpp
)

target_link_libraries(test_compression gtest lz4 zstd)
target_link_libraries(test_filer gtest lz4 zstd)
