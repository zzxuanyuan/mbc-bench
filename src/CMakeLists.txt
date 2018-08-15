include_directories(${PROJECT_SOURCE_DIR}/include)
message("include directory = " ${PROJECT_SOURCE_DIR}/include)

find_library(GTEST
	NAMES gtest
	PATH ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/build
)

find_library(ZSTD
	NAMES zstd
	PATH ${PROJECT_SOURCE_DIR}/lib/zstd/lib
)

find_library(LZ4
	NAMES lz4
	PATH ${PROJECT_SOURCE_DIR}/lib/lz4/lib
)

link_directories(${PROJECT_SOURCE_DIR}/lib/lz4/lib ${PROJECT_SOURCE_DIR}/lib/zstd/lib)

add_executable(run
	main.cpp
)

target_link_libraries(run lz4 zstd)
